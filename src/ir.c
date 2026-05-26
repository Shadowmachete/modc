#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "ir.h"
#include "modc_types.h"
#include "str.h"
#include "utils.h"
#include "vec.h"

#define STR_FMT_UNWRAP(str) (int)(str)->len, (str)->data

static Arena ir_arena;
static int ir_arena_init = 0;

void ir_memory_init(void) {
  if (!ir_arena_init) {
    arena_init(&ir_arena, sizeof(IrInstr) * 4096);
    ir_arena_init = 1;
  }
}

void ir_memory_release(void) {
  if (ir_arena_init) {
    ir_arena_init = 0;
    arena_clear(&ir_arena);
  }
}

IrInstr *ir_create_instr_array(size_t len) {
  return (IrInstr *)arena_alloc(&ir_arena, sizeof(IrInstr) * len);
}

IrGlobalVar *ir_create_global_array(size_t len) {
  IrGlobalVar *globals =
      (IrGlobalVar *)arena_alloc(&ir_arena, sizeof(IrGlobalVar) * len);

  if (!globals) {
    error(0, "arena alloc failed creating ir global array of length %zu", len);
    exit(1);
  }

  memset(globals, 0, sizeof(IrGlobalVar) * len);

  return globals;
}

IrFunc *ir_func_create(void) {
  IrFunc *func = (IrFunc *)arena_alloc(&ir_arena, sizeof(IrFunc));

  if (!func) {
    error(0, "arena alloc failed creating ir func");
    exit(1);
  }

  memset(func, 0, sizeof(IrFunc));

  return func;
}

IrFunc **ir_create_func_array(size_t len) {
  IrFunc **funcs = (IrFunc **)arena_alloc(&ir_arena, sizeof(IrFunc *) * len);

  if (!funcs) {
    error(0, "arena alloc failed creating ir func array of length %zu", len);
    exit(1);
  }

  memset(funcs, 0, sizeof(IrFunc *) * len);

  return funcs;
}

IrProgram *ir_program_create(void) {
  IrProgram *program = (IrProgram *)arena_alloc(&ir_arena, sizeof(IrProgram));

  if (!program) {
    error(0, "arena alloc failed creating ir program");
    exit(1);
  }

  memset(program, 0, sizeof(IrProgram));

  return program;
}

static int vec_ir_func_equality(void *f1, void *f2) {
  return str_eq(((IrFunc *)f1)->name, ((IrFunc *)f2)->name);
}

static int vec_ir_global_equality(void *g1, void *g2) {
  return str_eq(((IrGlobalVar *)g1)->name, ((IrGlobalVar *)g2)->name);
}

VecType vec_ir_func_ptr_type = {
    .to_string = NULL,
    .equals = vec_ir_func_equality,
    .free = NULL,
    .type_str = "IrFunc *",
};

VecType vec_ir_global_type = {
    .to_string = NULL,
    .equals = vec_ir_global_equality,
    .free = NULL,
    .type_str = "IrGlobalVar",
};

IrCtx *ir_ctx_create(void) {
  IrCtx *ctx = (IrCtx *)arena_alloc(&ir_arena, sizeof(IrCtx));

  if (!ctx) {
    error(0, "arena alloc failed creating ir ctx");
    exit(1);
  }

  memset(ctx, 0, sizeof(IrCtx));

  ctx->next_label = 1;
  ctx->next_temp = 1;
  ctx->break_label = -1;
  ctx->continue_label = -1;

  ctx->instr_buf.data = malloc(sizeof(IrInstr) * 64);
  ctx->instr_buf.cap = 64;
  ctx->instr_buf.size = 0;

  ctx->funcs_vec = vec_new(&vec_ir_func_ptr_type);
  ctx->globals_vec = vec_new(&vec_ir_global_type);

  return ctx;
}

IrOperand ir_const_int(i64 val, ModCType *type) {
  return (IrOperand){
      .variant = IR_OP_CONST_INT,
      .type = type,
      .number = val,
  };
}

IrOperand ir_const_float(f64 val, ModCType *type) {
  return (IrOperand){
      .variant = IR_OP_CONST_FLOAT,
      .type = type,
      .fvalue = val,
  };
}

IrOperand ir_const_string(String *val, ModCType *type) {
  return (IrOperand){
      .variant = IR_OP_CONST_STRING,
      .type = type,
      .str_value = val,
  };
}

IrOperand ir_var(String *name, ModCType *type) {
  return (IrOperand){
      .variant = IR_OP_VAR,
      .type = type,
      .var_name = name,
  };
}

IrOperand ir_temp(int temp_id, ModCType *type) {
  return (IrOperand){
      .variant = IR_OP_TEMP,
      .type = type,
      .temp_id = temp_id,
  };
}

IrOperand ir_label_ref(int label_id) {
  return (IrOperand){
      .variant = IR_OP_LABEL,
      .type = NULL,
      .label_id = label_id,
  };
}

b8 ir_op_is_const(IrOperand ir_op) {
  return ir_op.variant == IR_OP_CONST_INT ||
         ir_op.variant == IR_OP_CONST_FLOAT ||
         ir_op.variant == IR_OP_CONST_STRING;
}

IrInstr ir_instr_binop(IrOp op, IrOperand dest, IrOperand left,
                       IrOperand right) {
  return (IrInstr){
      .op = op,
      .binop.dest = dest,
      .binop.left = left,
      .binop.right = right,
  };
}

IrInstr ir_instr_unop(IrOp op, IrOperand dest, IrOperand src) {
  return (IrInstr){
      .op = op,
      .unop.dest = dest,
      .unop.src = src,
  };
}

IrInstr ir_instr_copy(IrOperand dest, IrOperand src) {
  return (IrInstr){
      .op = IR_COPY,
      .copy.dest = dest,
      .copy.src = src,
  };
}

IrInstr ir_instr_cast(IrOperand dest, IrOperand src, ModCType *to) {
  return (IrInstr){
      .op = IR_CAST,
      .cast.dest = dest,
      .cast.src = src,
      .cast.to = to,
  };
}

IrInstr ir_instr_addr_of(IrOperand dest, IrOperand var) {
  return (IrInstr){
      .op = IR_ADDR_OF,
      .addr_of.dest = dest,
      .addr_of.var = var,
  };
}

IrInstr ir_instr_load(IrOperand dest, IrOperand ptr) {
  return (IrInstr){
      .op = IR_LOAD,
      .load.dest = dest,
      .load.ptr = ptr,
  };
}

IrInstr ir_instr_store(IrOperand ptr, IrOperand src) {
  return (IrInstr){
      .op = IR_STORE,
      .store.ptr = ptr,
      .store.src = src,
  };
}

IrInstr ir_instr_index(IrOperand dest, IrOperand base, IrOperand idx) {
  return (IrInstr){
      .op = IR_INDEX,
      .index.dest = dest,
      .index.base = base,
      .index.idx = idx,
  };
}

IrInstr ir_instr_label(int label_id) {
  return (IrInstr){
      .op = IR_LABEL,
      .label.label_id = label_id,
  };
}

IrInstr ir_instr_jump(int label_id) {
  return (IrInstr){
      .op = IR_JUMP,
      .jump.label_id = label_id,
  };
}

IrInstr ir_instr_branch(IrOperand cond, int true_label, int false_label) {
  return (IrInstr){
      .op = IR_BRANCH,
      .branch.cond = cond,
      .branch.true_label = true_label,
      .branch.false_label = false_label,
  };
}

IrInstr ir_instr_switch(IrOperand val, int *case_labels, size_t *case_values,
                        size_t count, int default_label) {
  return (IrInstr){
      .op = IR_SWITCH,
      .switch_instr.val = val,
      .switch_instr.case_values = case_values,
      .switch_instr.case_labels = case_labels,
      .switch_instr.case_count = count,
      .switch_instr.default_label = default_label,
  };
}

IrInstr ir_instr_param(IrOperand arg) {
  return (IrInstr){
      .op = IR_PARAM,
      .param.arg = arg,
  };
}

IrInstr ir_instr_call(IrOperand dest, IrOperand callee, int arg_count) {
  return (IrInstr){
      .op = IR_CALL,
      .call.dest = dest,
      .call.callee = callee,
      .call.arg_count = arg_count,
  };
}

IrInstr ir_instr_return(IrOperand *val) {
  return (IrInstr){
      .op = IR_RETURN,
      .ret.val = val != NULL ? *val : (IrOperand){0},
      .ret.has_val = val != NULL ? 1 : 0,
  };
}

int ir_next_temp(IrCtx *ctx) { return ctx->next_temp++; }

int ir_next_label(IrCtx *ctx) { return ctx->next_label++; }

void ir_emit(IrCtx *ctx, IrInstr instr) {
  if (ctx->instr_buf.size + 1 >= ctx->instr_buf.cap) {
    ctx->instr_buf.cap *= 2;
    ctx->instr_buf.data =
        realloc(ctx->instr_buf.data, ctx->instr_buf.cap * sizeof(IrInstr));
  }

  ctx->instr_buf.data[ctx->instr_buf.size++] = instr;
}

IrOperand ir_gen_expr(IrCtx *ctx, Ast *ast) {
  switch (ast->variant) {
  case AST_INTEGER:
  case AST_FLOAT:
  case AST_STRING:
  case AST_BOOL: {
    return ir_gen_literal(ctx, ast);
  }
  case AST_IDENTIFIER: {
    return ir_gen_ident(ctx, ast);
  }
  case AST_UNOP: {
    return ir_gen_unop(ctx, ast);
  }
  case AST_BINOP: {
    return ir_gen_binop(ctx, ast);
  }
  case AST_CAST: {
    return ir_gen_cast(ctx, ast);
  }
  case AST_FUNCALL:
  case AST_FUNPTR_CALL: {
    return ir_gen_func_call(ctx, ast);
  }
  default: {
    // never
    return (IrOperand){0};
  }
  }
}

IrOperand ir_gen_literal(IrCtx *ctx, Ast *ast) {
  (void)ctx;
  switch (ast->variant) {
  case AST_INTEGER:
  case AST_BOOL: {
    return ir_const_int(ast->number.value, ast->type);
  } break;
  case AST_FLOAT: {
    return ir_const_float(ast->fvalue.value, ast->type);
  } break;
  case AST_STRING: {
    return ir_const_string(ast->string.value, ast->type);
  } break;
  default: {
    // never
    return (IrOperand){0};
  }
  }
}

IrOperand ir_gen_ident(IrCtx *ctx, Ast *ast) {
  (void)ctx;
  IrOperand var = ir_var(ast->ident.name, ast->type);
  return var;
}

IrOperand ir_gen_unop(IrCtx *ctx, Ast *ast) {
  IrOperand expr = ir_gen_expr(ctx, ast->unary.expr);
  switch (ast->unary.unop) {
  case AST_UNOP_POST_INC: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    IrOperand temp_2 = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_copy(temp, expr));
    ir_emit(ctx, ir_instr_copy(temp_2, expr));
    ir_emit(ctx,
            ir_instr_binop(IR_ADD, temp_2, temp_2, ir_const_int(1, ast->type)));
    ir_emit(ctx, ir_instr_copy(expr, temp_2));

    return temp;
  } break;
  case AST_UNOP_POST_DEC: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    IrOperand temp_2 = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_copy(temp, expr));
    ir_emit(ctx, ir_instr_copy(temp_2, expr));
    ir_emit(ctx,
            ir_instr_binop(IR_SUB, temp_2, temp_2, ir_const_int(1, ast->type)));
    ir_emit(ctx, ir_instr_copy(expr, temp_2));

    return temp;
  } break;
  case AST_UNOP_PRE_INC: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_copy(temp, expr));
    ir_emit(ctx,
            ir_instr_binop(IR_ADD, temp, temp, ir_const_int(1, ast->type)));
    ir_emit(ctx, ir_instr_copy(expr, temp));

    return temp;
  } break;
  case AST_UNOP_PRE_DEC: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_copy(temp, expr));
    ir_emit(ctx,
            ir_instr_binop(IR_SUB, temp, temp, ir_const_int(1, ast->type)));
    ir_emit(ctx, ir_instr_copy(expr, temp));

    return temp;
  } break;
  case AST_UNOP_PLUS: {
    return expr;
  } break;
  case AST_UNOP_MINUS: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_unop(IR_NEG, temp, expr));

    return temp;
  } break;
  case AST_UNOP_LOG_NOT: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_unop(IR_LOG_NOT, temp, expr));

    return temp;
  } break;
  case AST_UNOP_BIT_NOT: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_unop(IR_BIT_NOT, temp, expr));

    return temp;
  } break;
  case AST_UNOP_ADDR_OF: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_addr_of(temp, expr));

    return temp;
  } break;
  case AST_UNOP_DEREF: {
    IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_load(temp, expr));

    return temp;
  } break;
  }

  return (IrOperand){0};
}

IrOperand ir_gen_binop(IrCtx *ctx, Ast *ast) {
  IrOperand left;
  IrOperand var;

  b8 left_is_unop_deref = ast->binary.left->variant == AST_UNOP &&
                          ast->binary.left->unary.unop == AST_UNOP_DEREF;

  if (left_is_unop_deref && binop_is_assignment(ast->binary.binop)) {
    // TODO: verify correctness when pointer support is added
    assert(0 && "pointers are not supported yet");
  } else {
    left = ir_gen_expr(ctx, ast->binary.left);
  }
  IrOperand right = ir_gen_expr(ctx, ast->binary.right);

  if (left.variant == IR_OP_VAR && ast->binary.binop != AST_BIN_OP_ASSIGN) {
    var = left;
    left = ir_temp(ir_next_temp(ctx), var.type);
    ir_emit(ctx, ir_instr_copy(left, var));
  }

  if (right.variant == IR_OP_VAR) {
    IrOperand tmp_ = right;
    right = ir_temp(ir_next_temp(ctx), tmp_.type);
    ir_emit(ctx, ir_instr_copy(right, tmp_));
  }

  if (ir_op_is_const(left) && !ir_op_is_const(right)) {
    IrOperand tmp_ = right;
    right = left;
    left = tmp_;
  }

  IrOperand temp;
  switch (ast->binary.binop) {
  case AST_BIN_OP_ADD: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_ADD, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_SUB: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_SUB, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_MUL: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_MUL, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_DIV: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_DIV, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_MOD: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_MOD, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_SHL: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_SHL, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_SHR: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_SHR, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_LT: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_LT, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_LE: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_LE, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_GT: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_GT, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_GE: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_GE, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_EQ: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_EQ, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_NE: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_NE, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_BIT_AND: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_BIT_AND, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_BIT_XOR: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_BIT_XOR, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_BIT_OR: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);
    ir_emit(ctx, ir_instr_binop(IR_BIT_OR, temp, left, right));

    return temp;
  } break;
  case AST_BIN_OP_LOG_AND: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);

    int l_check_right = ir_next_label(ctx);
    int l_true = ir_next_label(ctx);
    int l_false = ir_next_label(ctx);
    int l_end = ir_next_label(ctx);

    ir_emit(ctx, ir_instr_branch(left, l_check_right, l_false));

    ir_emit(ctx, ir_instr_label(l_check_right));
    ir_emit(ctx, ir_instr_branch(right, l_true, l_false));

    ir_emit(ctx, ir_instr_label(l_true));
    ir_emit(ctx, ir_instr_copy(temp, ir_const_int(1, ast->type)));
    ir_emit(ctx, ir_instr_jump(l_end));

    ir_emit(ctx, ir_instr_label(l_false));
    ir_emit(ctx, ir_instr_copy(temp, ir_const_int(0, ast->type)));

    ir_emit(ctx, ir_instr_label(l_end));

    return temp;
  } break;
  case AST_BIN_OP_LOG_OR: {
    temp = ir_temp(ir_next_temp(ctx), ast->type);

    int l_check_right = ir_next_label(ctx);
    int l_false = ir_next_label(ctx);
    int l_true = ir_next_label(ctx);
    int l_end = ir_next_label(ctx);

    ir_emit(ctx, ir_instr_branch(left, l_true, l_check_right));

    ir_emit(ctx, ir_instr_label(l_check_right));
    ir_emit(ctx, ir_instr_branch(right, l_true, l_false));

    ir_emit(ctx, ir_instr_label(l_true));
    ir_emit(ctx, ir_instr_copy(temp, ir_const_int(1, ast->type)));
    ir_emit(ctx, ir_instr_jump(l_end));

    ir_emit(ctx, ir_instr_label(l_false));
    ir_emit(ctx, ir_instr_copy(temp, ir_const_int(0, ast->type)));

    ir_emit(ctx, ir_instr_label(l_end));

    return temp;
  } break;
  case AST_BIN_OP_ASSIGN: {
    if (left_is_unop_deref) {
      ir_emit(ctx, ir_instr_store(left, right));
    } else {
      ir_emit(ctx, ir_instr_copy(left, right));
    }

    return left;
  } break;
  case AST_BIN_OP_ADD_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_ADD, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_ADD, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_SUB_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_SUB, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_SUB, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_MUL_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_MUL, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_MUL, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_DIV_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_DIV, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_DIV, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_MOD_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_MOD, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_MOD, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_SHL_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_SHL, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_SHL, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_SHR_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_SHR, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_SHR, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_AND_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_BIT_AND, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_BIT_AND, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_XOR_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_BIT_XOR, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_BIT_XOR, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  case AST_BIN_OP_OR_ASSIGN: {
    if (left_is_unop_deref) {
      temp = ir_temp(ir_next_temp(ctx), ast->type);
      ir_emit(ctx, ir_instr_binop(IR_BIT_OR, temp, left, right));
      ir_emit(ctx, ir_instr_store(left, temp));
      return left;
    } else {
      ir_emit(ctx, ir_instr_binop(IR_BIT_OR, left, left, right));
      ir_emit(ctx, ir_instr_copy(var, left));
      return var;
    }
  } break;
  }

  return left;
}

IrOperand ir_gen_cast(IrCtx *ctx, Ast *ast) {
  // NOTE: I am very unsure about this casting

  IrOperand expr = ir_gen_expr(ctx, ast->type_cast.expr);
  return expr;

  IrOperand temp = ir_temp(ir_next_temp(ctx), ast->type_cast.type);

  ir_emit(ctx, ir_instr_cast(temp, expr, ast->type_cast.type));

  return temp;
}

IrOperand ir_gen_func_call(IrCtx *ctx, Ast *ast) {
  for (size_t i = 0; i < ast->func_call.val_count; i++) {
    ir_emit(ctx, ir_instr_param(ir_gen_expr(ctx, ast->func_call.values[i])));
  }

  b8 returns_void = ast->type->variant == TYPE_VOID;

  IrOperand callee = ir_gen_expr(ctx, ast->func_call.callee);
  IrOperand temp =
      returns_void ? (IrOperand){0} : ir_temp(ir_next_temp(ctx), ast->type);

  ir_emit(ctx, ir_instr_call(temp, callee, ast->func_call.val_count));

  return temp;
}

void ir_gen_stmt(IrCtx *ctx, Ast *ast) {
  switch (ast->variant) {
  case AST_INTEGER:
  case AST_FLOAT:
  case AST_STRING:
  case AST_BOOL:
  case AST_IDENTIFIER:
  case AST_UNOP:
  case AST_BINOP:
  case AST_FUNCALL:
  case AST_FUNPTR_CALL:
  case AST_CAST: {
    ir_gen_expr(ctx, ast);
  } break;
  case AST_VAR: {
    ir_gen_var_decl(ctx, ast);
  } break;
  case AST_FUNC: {
    ir_gen_func_decl(ctx, ast);
  } break;
  case AST_FUNPTR: {
  } break;
  case AST_BLOCK: {
    ir_gen_block(ctx, ast);
  } break;
  case AST_IF: {
    ir_gen_if(ctx, ast);
  } break;
  case AST_FOR: {
    ir_gen_for(ctx, ast);
  } break;
  case AST_WHILE: {
    ir_gen_while(ctx, ast);
  } break;
  case AST_RETURN: {
    ir_gen_return(ctx, ast);
  } break;
  case AST_BREAK: {
    ir_gen_break(ctx, ast);
  } break;
  case AST_CONTINUE: {
    ir_gen_continue(ctx, ast);
  } break;
  case AST_SWITCH: {
  } break;
  case AST_CASE: {
  } break;
  case AST_JUMP: {
  } break;
  }
}

void ir_gen_block(IrCtx *ctx, Ast *ast) {
  for (size_t i = 0; i < ast->block.stmt_count; i++) {
    ir_gen_stmt(ctx, ast->block.statements[i]);
  }
}

void ir_gen_var_decl(IrCtx *ctx, Ast *ast) {
  if (ast->var_decl.is_global) {
    // TODO: populate into the globals struct of the program
    return;
  }

  if (ast->var_decl.initializer != NULL) {
    IrOperand temp = ir_gen_expr(ctx, ast->var_decl.initializer);
    IrOperand var = ir_var(ast->var_decl.name, ast->var_decl.type);
    ir_emit(ctx, ir_instr_copy(var, temp));
  }
}

void ir_gen_if(IrCtx *ctx, Ast *ast) {
  /*
   *
   * cond
   * branch cond
   * jmp L true
   * jmp L false
   * L true
   * then
   * jmp L end
   * L false
   * els
   * L end
   *
   */

  b8 has_els = ast->if_stmt.els != NULL;

  int l_true = ir_next_label(ctx);
  int l_false = ir_next_label(ctx);
  int l_end = has_els ? ir_next_label(ctx) : l_false;

  IrOperand cond = ir_gen_expr(ctx, ast->if_stmt.cond);

  ir_emit(ctx, ir_instr_branch(cond, l_true, l_false));

  ir_emit(ctx, ir_instr_label(l_true));

  ir_gen_block(ctx, ast->if_stmt.then);

  if (has_els) {
    ir_emit(ctx, ir_instr_jump(l_end));

    ir_emit(ctx, ir_instr_label(l_false));

    ir_gen_block(ctx, ast->if_stmt.els);
  }

  ir_emit(ctx, ir_instr_label(l_end));
}

void ir_gen_for(IrCtx *ctx, Ast *ast) {
  /*
   *
   * init
   * L cond
   * branch cond
   * jmp L body
   * jmp L end
   * L body
   * body
   * step
   * jmp L cond
   * L end
   *
   */

  int l_cond = ir_next_label(ctx);
  int l_body = ir_next_label(ctx);
  int l_end = ir_next_label(ctx);

  ctx->break_label = l_end;
  ctx->continue_label = l_cond;

  ir_gen_stmt(ctx, ast->for_stmt.forinit);

  ir_emit(ctx, ir_instr_label(l_cond));

  IrOperand cond = ir_gen_expr(ctx, ast->for_stmt.forcond);

  ir_emit(ctx, ir_instr_branch(cond, l_body, l_end));

  ir_emit(ctx, ir_instr_label(l_body));

  ir_gen_block(ctx, ast->for_stmt.forbody);

  ir_gen_stmt(ctx, ast->for_stmt.forstep);

  ir_emit(ctx, ir_instr_jump(l_cond));

  ir_emit(ctx, ir_instr_label(l_end));
}

void ir_gen_while(IrCtx *ctx, Ast *ast) {
  /*
   *
   * L cond
   * branch cond
   * jmp L body
   * jmp L end
   * L body
   * body
   * jmp L cond
   * L end
   *
   */

  int l_cond = ir_next_label(ctx);
  int l_body = ir_next_label(ctx);
  int l_end = ir_next_label(ctx);

  ctx->break_label = l_end;
  ctx->continue_label = l_cond;

  ir_emit(ctx, ir_instr_label(l_cond));

  IrOperand cond = ir_gen_expr(ctx, ast->while_stmt.whilecond);

  ir_emit(ctx, ir_instr_branch(cond, l_body, l_end));

  ir_emit(ctx, ir_instr_label(l_body));

  ir_gen_block(ctx, ast->while_stmt.whilebody);

  ir_emit(ctx, ir_instr_jump(l_cond));

  ir_emit(ctx, ir_instr_label(l_end));
}

void ir_gen_switch(IrCtx *ctx, Ast *ast);

void ir_gen_return(IrCtx *ctx, Ast *ast) {
  IrOperand ret = ir_gen_expr(ctx, ast->return_stmt.expr);
  ir_emit(ctx, ir_instr_return(ast->return_stmt.expr != NULL ? &ret : NULL));
}

void ir_gen_break(IrCtx *ctx, Ast *ast) {
  if (ctx->break_label == -1) {
    error(ast->line, "uncaught error: break called otside of loop.");
    check_errors();
  }

  ir_emit(ctx, ir_instr_jump(ctx->break_label));
}

void ir_gen_continue(IrCtx *ctx, Ast *ast) {
  if (ctx->continue_label == -1) {
    error(ast->line, "uncaught error: continue called otside of loop.");
    check_errors();
  }

  ir_emit(ctx, ir_instr_jump(ctx->continue_label));
}

IrFunc *ir_gen_func_decl(IrCtx *ctx, Ast *ast) {
  IrFunc *func = ir_func_create();
  func->name = ast->func_decl.name;
  func->type = ast->func_decl.func_type;

  func->param_count = ast->func_decl.param_count;
  func->param_names =
      (String **)global_arena_alloc(sizeof(String *) * func->param_count);
  memset(func->param_names, 0, sizeof(String *) * func->param_count);

  for (size_t i = 0; i < ast->func_decl.param_count; i++) {
    func->param_names[i] = ast->func_decl.params[i]->var_decl.name;
  }

  ctx->current_func = func;

  // TODO: generate function prologue
  warning(ast->line, "Function prologue has not been developed yet. The IR "
                     "generated is not proper");

  ir_gen_block(ctx, ast->func_decl.body);

  // TODO: generate function epilogue
  warning(ast->line, "Function epilogue has not been developed yet. The IR "
                     "generated is not proper");

  func->instr_count = ctx->instr_buf.size;
  func->instrs = ir_create_instr_array(func->instr_count);
  memcpy(func->instrs, ctx->instr_buf.data,
         func->instr_count * sizeof(IrInstr));

  vec_push(ctx->funcs_vec, (void *)func);

  ctx->instr_buf.size = 0;

  return func;
}

IrProgram *ir_gen(Ast *global_ast) {
  IrProgram *program = ir_program_create();
  IrCtx *ctx = ir_ctx_create();
  ctx->program = program;
  ctx->current_func = NULL;

  ir_gen_block(ctx, global_ast);

  program->func_count = ctx->funcs_vec->size;
  program->funcs = ir_create_func_array(program->func_count);
  memcpy(program->funcs, ctx->funcs_vec->items,
         program->func_count * sizeof(IrFunc *));

  program->global_count = ctx->globals_vec->size;
  program->globals = ir_create_global_array(program->global_count);
  memcpy(program->globals, ctx->globals_vec->items,
         program->global_count * sizeof(IrGlobalVar));

  vec_release(ctx->funcs_vec);
  vec_release(ctx->globals_vec);
  free(ctx->instr_buf.data);

  return program;
}

void ir_operand_display(IrOperand operand) {
  switch (operand.variant) {
  case IR_OP_CONST_INT: {
    printf("%" PRId64 "", operand.number);
  } break;
  case IR_OP_CONST_FLOAT: {
    printf("%lf", operand.fvalue);
  } break;
  case IR_OP_CONST_STRING: {
    printf("%.*s", STR_FMT_UNWRAP(operand.str_value));
  } break;
  case IR_OP_VAR: {
    printf("%.*s", STR_FMT_UNWRAP(operand.var_name));
  } break;
  case IR_OP_TEMP: {
    printf("t%d", operand.temp_id);
  } break;
  case IR_OP_LABEL: {
    printf("L%d", operand.label_id);
  } break;
  }
}

void ir_instr_display(IrInstr instr) {
  switch (instr.op) {
  case IR_ADD: {
    printf("ADD ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_SUB: {
    printf("SUB ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_MUL: {
    printf("MUL ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_DIV: {
    printf("DIV ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_MOD: {
    printf("MOD ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_NEG: {
    printf("NEG ");
    ir_operand_display(instr.unop.dest);
    printf(", ");
    ir_operand_display(instr.unop.src);
    printf("\n");
  } break;
  case IR_SHL: {
    printf("SHL ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_SHR: {
    printf("SHR ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_BIT_AND: {
    printf("AND ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_BIT_OR: {
    printf("OR ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_BIT_XOR: {
    printf("XOR ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_BIT_NOT: {
    printf("NOT ");
    ir_operand_display(instr.unop.dest);
    printf(", ");
    ir_operand_display(instr.unop.src);
    printf("\n");
  } break;
  case IR_LOG_NOT: {
    printf("LOG NOT ");
    ir_operand_display(instr.unop.dest);
    printf(", ");
    ir_operand_display(instr.unop.src);
    printf("\n");
  } break;
  case IR_LT: {
    printf("LT ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_LE: {
    printf("LE ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_GT: {
    printf("GT ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_GE: {
    printf("GE ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_EQ: {
    printf("EQ ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_NE: {
    printf("NE ");
    ir_operand_display(instr.binop.dest);
    printf(", ");
    ir_operand_display(instr.binop.left);
    printf(", ");
    ir_operand_display(instr.binop.right);
    printf("\n");
  } break;
  case IR_COPY: {
    printf("COPY ");
    ir_operand_display(instr.copy.dest);
    printf(", ");
    ir_operand_display(instr.copy.src);
    printf("\n");
  } break;
  case IR_CAST: {
    printf("CAST ");
    ir_operand_display(instr.cast.dest);
    printf(", ");
    ir_operand_display(instr.cast.src);
    printf(", %.*s", STR_FMT_UNWRAP(modctype_to_string(instr.cast.to)));
    printf("\n");
  } break;
  case IR_ADDR_OF: {
    printf("ADDR ");
    ir_operand_display(instr.cast.dest);
    printf(", ");
    ir_operand_display(instr.copy.src);
    printf("\n");
  } break;
  case IR_LOAD: {
    printf("LOAD ");
    ir_operand_display(instr.cast.dest);
    printf(", ");
    ir_operand_display(instr.copy.src);
    printf("\n");
  } break;
  case IR_STORE: {
    printf("STORE ");
    ir_operand_display(instr.cast.dest);
    printf(", ");
    ir_operand_display(instr.copy.src);
    printf("\n");
  } break;
  case IR_INDEX: {
  } break;
  case IR_FIELD: {
  } break;
  case IR_LABEL: {
    printf("L%d:\n", instr.label.label_id);
  } break;
  case IR_JUMP: {
    printf("JMP L%d\n", instr.jump.label_id);
  } break;
  case IR_BRANCH: {
    printf("BRANCH ");
    ir_operand_display(instr.branch.cond);
    printf(", L%d, L%d\n", instr.branch.true_label, instr.branch.false_label);
  } break;
  case IR_SWITCH: {
  } break;
  case IR_PARAM: {
    printf("ARG ");
    ir_operand_display(instr.param.arg);
    printf("\n");
  } break;
  case IR_CALL: {
    printf("CALL ");
    ir_operand_display(instr.call.dest);
    printf(", ");
    ir_operand_display(instr.call.callee);
    printf("\n");
  } break;
  case IR_RETURN: {
    printf("RET ");
    if (instr.ret.has_val)
      ir_operand_display(instr.ret.val);

    printf("\n");
  } break;
  }
}

void ir_display(IrProgram *prog) {
  for (size_t i = 0; i < prog->global_count; i++) {
    IrGlobalVar global = prog->globals[i];
    printf("%.*s %.*s", STR_FMT_UNWRAP(modctype_to_string(global.type)),
           STR_FMT_UNWRAP(global.name));
    ir_operand_display(global.initializer);
    printf("\n");
  }

  for (size_t i = 0; i < prog->func_count; i++) {
    IrFunc *func = prog->funcs[i];
    printf("%.*s %.*s(", STR_FMT_UNWRAP(modctype_to_string(func->type)),
           STR_FMT_UNWRAP(func->name));

    for (size_t j = 0; j < func->param_count; j++) {
      printf("%.*s", STR_FMT_UNWRAP(func->param_names[j]));
      if (j != func->param_count - 1)
        printf(", ");
    }

    printf(")\n");

    printf("[prologue goes here]\n");

    for (size_t j = 0; j < func->instr_count; j++) {
      ir_instr_display(func->instrs[j]);
    }

    printf("[epilogue goes here]\n");
  }
}
