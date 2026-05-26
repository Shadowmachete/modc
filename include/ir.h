#ifndef IR_H
#define IR_H

#include "ast.h"
#include "modc_types.h"

typedef enum {
  IR_OP_CONST_INT,
  IR_OP_CONST_FLOAT,
  IR_OP_CONST_STRING,
  IR_OP_VAR,
  IR_OP_TEMP,
  IR_OP_LABEL,
} IrOperandVariant;

typedef struct {
  IrOperandVariant variant;
  ModCType *type;
  union {
    i64 number; // integer / bool / char
    f64 fvalue; // float
    String *str_value;
    String
        *var_name; // points into symbol table TODO: maybe change to String_View
    int temp_id;   // t0, t1, ...
    int label_id;  // L0, L1
  };
} IrOperand;

b8 ir_op_is_const(IrOperand ir_op);

typedef enum {
  // arithmetic
  IR_ADD,
  IR_SUB,
  IR_MUL,
  IR_DIV,
  IR_MOD,
  IR_NEG, // unary minus
  IR_SHL,
  IR_SHR,
  IR_BIT_AND,
  IR_BIT_OR,
  IR_BIT_XOR,
  IR_BIT_NOT,

  // && and || can be converted into branches, so we just need the !
  IR_LOG_NOT,

  // comparison
  IR_LT,
  IR_LE,
  IR_GT,
  IR_GE,
  IR_EQ,
  IR_NE,

  IR_COPY,
  IR_CAST,

  // memory access
  IR_ADDR_OF, // dest = &var
  IR_LOAD,    // dest = *ptr
  IR_STORE,   // *ptr = src
  IR_INDEX,   // dest = base[index]
  IR_FIELD,   // dest = base.field_offset

  // control flow
  IR_LABEL,
  IR_JUMP,   // unconditional goto
  IR_BRANCH, // basically an if else
  IR_SWITCH, // goes to switch table

  // funcs
  IR_PARAM,
  IR_CALL,
  IR_RETURN,
} IrOp;

typedef struct {
  IrOp op;
  union {
    struct {
      IrOperand dest;
      IrOperand left;
      IrOperand right;
    } binop;

    struct {
      IrOperand dest;
      IrOperand src;
    } unop;

    struct {
      IrOperand dest;
      IrOperand src;
    } copy;

    struct {
      IrOperand dest;
      IrOperand src;
      ModCType *to;
    } cast;

    struct {
      IrOperand dest;
      IrOperand var;
    } addr_of;

    struct {
      IrOperand dest;
      IrOperand ptr;
    } load;

    struct {
      IrOperand ptr;
      IrOperand src;
    } store;

    struct {
      IrOperand dest;
      IrOperand base;
      IrOperand idx;
    } index;

    struct {
      int label_id;
    } label;

    struct {
      int label_id;
    } jump;

    struct {
      IrOperand cond;
      int true_label;
      int false_label;
    } branch;

    struct {
      IrOperand val;
      size_t *case_values; // array of case constants
      int *case_labels;    // parallel array of label ids
      size_t case_count;
      int default_label;
    } switch_instr;

    struct {
      IrOperand arg;
    } param;

    struct {
      IrOperand dest;
      IrOperand callee;
      int arg_count;
    } call;

    struct {
      IrOperand val;
      b8 has_val;
    } ret;
  };
} IrInstr;

typedef struct {
  String *name; // TODO: change to String_View
  ModCType *type;
  String **param_names;
  size_t param_count;

  IrInstr *instrs;
  size_t instr_count;
} IrFunc;

typedef struct {
  String *name;
  ModCType *type;
  IrOperand initializer; // must be a IR_OP_CONST
  b8 is_zero_init;       // true if no initializer given, zero-fill
} IrGlobalVar;

typedef struct {
  IrFunc **funcs;
  size_t func_count;

  IrGlobalVar *globals;
  size_t global_count;
} IrProgram;

typedef struct {
  IrProgram *program;
  IrFunc *current_func;

  struct {
    IrInstr *data;
    size_t size;
    size_t cap;
  } instr_buf;

  Vec *funcs_vec;
  Vec *globals_vec;

  int next_temp;
  int next_label;
  // to jump to if AST_BREAK or AST_CONTINUE is encountered, -1 if not in a loop
  int break_label;
  int continue_label;
} IrCtx;

void ir_memory_init(void);
void ir_memory_release(void);
IrInstr *ir_create_instr_array(size_t len);
IrFunc *ir_func_create(void);
IrProgram *ir_program_create(void);
IrCtx *ir_ctx_create(void);

IrOperand ir_const_int(i64 val, ModCType *type);
IrOperand ir_const_float(f64 val, ModCType *type);
IrOperand ir_const_string(String *val, ModCType *type);
IrOperand ir_temp(int temp_id, ModCType *type);
IrOperand ir_var(String *name, ModCType *type);
IrOperand ir_label_ref(int label_id);

IrInstr ir_instr_binop(IrOp op, IrOperand dest, IrOperand left,
                       IrOperand right);
IrInstr ir_instr_unop(IrOp op, IrOperand dest, IrOperand src);
IrInstr ir_instr_copy(IrOperand dest, IrOperand src);
IrInstr ir_instr_cast(IrOperand dest, IrOperand src, ModCType *to);
IrInstr ir_instr_addr_of(IrOperand dest, IrOperand var);
IrInstr ir_instr_load(IrOperand dest, IrOperand ptr);
IrInstr ir_instr_store(IrOperand ptr, IrOperand src);
IrInstr ir_instr_index(IrOperand dest, IrOperand base, IrOperand idx);
IrInstr ir_instr_label(int label_id);
IrInstr ir_instr_jump(int label_id);
IrInstr ir_instr_branch(IrOperand cond, int true_label, int false_label);
IrInstr ir_instr_switch(IrOperand val, int *case_labels, size_t *case_values,
                        size_t count, int default_label);
IrInstr ir_instr_param(IrOperand arg);
IrInstr ir_instr_call(IrOperand dest, IrOperand callee, int arg_count);
IrInstr ir_instr_return(IrOperand *val); // val is NULL for void return

int ir_next_temp(IrCtx *ctx);            // returns next temp id
int ir_next_label(IrCtx *ctx);           // returns next label id
void ir_emit(IrCtx *ctx, IrInstr instr); // appends to current function

IrOperand ir_gen_expr(IrCtx *ctx, Ast *ast);
IrOperand ir_gen_literal(IrCtx *ctx, Ast *ast);
IrOperand ir_gen_ident(IrCtx *ctx, Ast *ast);
IrOperand ir_gen_unop(IrCtx *ctx, Ast *ast);
IrOperand ir_gen_binop(IrCtx *ctx, Ast *ast); // handles compound assigns too
IrOperand ir_gen_cast(IrCtx *ctx, Ast *ast);
IrOperand ir_gen_func_call(IrCtx *ctx, Ast *ast);

void ir_gen_stmt(IrCtx *ctx, Ast *ast);
void ir_gen_block(IrCtx *ctx, Ast *ast);
void ir_gen_var_decl(IrCtx *ctx, Ast *ast);
void ir_gen_if(IrCtx *ctx, Ast *ast);
void ir_gen_for(IrCtx *ctx, Ast *ast);
void ir_gen_while(IrCtx *ctx, Ast *ast);
void ir_gen_switch(IrCtx *ctx, Ast *ast);
void ir_gen_return(IrCtx *ctx, Ast *ast);
void ir_gen_break(IrCtx *ctx, Ast *ast);
void ir_gen_continue(IrCtx *ctx, Ast *ast);

IrFunc *ir_gen_func_decl(IrCtx *ctx, Ast *ast);
IrProgram *ir_gen(Ast *global_ast);

void ir_display(IrProgram *prog);

#endif // !IR_H
