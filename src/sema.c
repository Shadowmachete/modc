#include "sema.h"
#include "ast.h"
#include "hash_map.h"
#include "modc_types.h"
#include "scope.h"
#include "str.h"
#include "utils.h"
#include "vec.h"

#define STR_FMT_UNWRAP(str) (int)(str)->len, (str)->data

void sema_binop(Ast *ast, Scope *current_scope, ModCType *return_type) {
  Ast *left = ast->binary.left;
  Ast *right = ast->binary.right;
  sema_check(left, current_scope, return_type);
  sema_check(right, current_scope, return_type);

  if (left->type == error_type || right->type == error_type) {
    ast->type = error_type;
    return;
  }

  /*
   * Handle other types:
   *
   * all defined in overload table?
   */

  String *binop_str = ast_binop_to_string(ast->binary.binop);
  String *left_type_str = modctype_to_string(ast->binary.left->type);
  String *right_type_str = modctype_to_string(ast->binary.right->type);

  if (!t_is_numeric(left->type) || !t_is_numeric(right->type)) {
    if (ast->binary.binop == AST_BIN_OP_ASSIGN) {
      if (left->type != right->type) {
        error(ast->line, "Cannot assign value of type %.*s to type %.*s",
              STR_FMT_UNWRAP(right_type_str), STR_FMT_UNWRAP(left_type_str));
        ast->type = error_type;
      }
      ast->type = left->type;
      return;
    }

    // TODO: check overloads table if operation is defined
    b8 op_defined = 0;

    if (!op_defined) {
      error(ast->line,
            "Binary operation \"%.*s\" is not defined between types %.*s and "
            "%.*s",
            STR_FMT_UNWRAP(binop_str), STR_FMT_UNWRAP(left_type_str),
            STR_FMT_UNWRAP(right_type_str));
      ast->type = error_type;
      return;
    }

    return;
  }

  /*
   * Handle booleans
   *
   * bool only defines && and ||
   */

  b8 left_is_bool = (left->type->variant == TYPE_BOOL);
  b8 right_is_bool = (right->type->variant == TYPE_BOOL);

  if (left_is_bool || right_is_bool) {
    if (ast->binary.binop != AST_BIN_OP_LOG_AND &&
        ast->binary.binop != AST_BIN_OP_LOG_OR) {
      error(ast->line,
            "Binary operation \"%.*s\" is not defined for type boolean",
            STR_FMT_UNWRAP(binop_str));
      ast->type = error_type;
      return;
    }

    ast->type = bool_type;
    return;
  }

  /*
   * TODO: Handle pointers
   */

  /*
   * Handle floats and integers
   *
   * mul, div, add, sub, log and, log or, comparison takes int/float for both
   * mod, shl, shr, bit and, bit xor, bit or take int for both
   * same for the ones with assignment
   *
   */

  switch (ast->binary.binop) {
  case AST_BIN_OP_ASSIGN: {
    if (left->variant != AST_IDENTIFIER ||
        find_symbol(current_scope, left->ident.name)->variant !=
            SYMBOL_VARIABLE) {
      error(ast->line, "Left hand side of an assignment is not a variable");
      ast->type = error_type;
      return;
    }

    if (!types_can_fit(left->type, right->type)) {
      error(ast->line, "Cannot implicitly cast the value of the right hand "
                       "side to the assigned type on the left");
      ast->type = error_type;
      return;
    }

    ast->type = left->type;
  } break;
  case AST_BIN_OP_MUL:
  case AST_BIN_OP_DIV:
  case AST_BIN_OP_ADD:
  case AST_BIN_OP_SUB: {
    ast->type = make_combined_type(ast);
  } break;
  case AST_BIN_OP_ADD_ASSIGN:
  case AST_BIN_OP_SUB_ASSIGN:
  case AST_BIN_OP_MUL_ASSIGN:
  case AST_BIN_OP_DIV_ASSIGN: {
    if (left->variant != AST_IDENTIFIER ||
        find_symbol(current_scope, left->ident.name)->variant !=
            SYMBOL_VARIABLE) {
      error(ast->line, "Left hand side of an assignment is not a variable");
      ast->type = error_type;
      return;
    }

    ModCType *combined_type = make_combined_type(ast);

    if (!types_can_fit(left->type, combined_type)) {
      error(ast->line, "Cannot implicitly cast the value of the right hand "
                       "side to the assigned type on the left");
      ast->type = error_type;
      return;
    }

    ast->type = void_type;
  } break;
  case AST_BIN_OP_LOG_AND:
  case AST_BIN_OP_LOG_OR:
  case AST_BIN_OP_LT:
  case AST_BIN_OP_LE:
  case AST_BIN_OP_GT:
  case AST_BIN_OP_GE:
  case AST_BIN_OP_EQ:
  case AST_BIN_OP_NE: {
    ast->type = bool_type;
  } break;
  case AST_BIN_OP_MOD:
  case AST_BIN_OP_SHL:
  case AST_BIN_OP_SHR:
  case AST_BIN_OP_BIT_AND:
  case AST_BIN_OP_BIT_XOR:
  case AST_BIN_OP_BIT_OR: {
    if (left->type->variant == TYPE_FLOAT ||
        right->type->variant == TYPE_FLOAT) {
      error(ast->line, "Binary operation %.*s is not defined for type float",
            STR_FMT_UNWRAP(binop_str));
      ast->type = error_type;
      return;
    }

    ast->type = make_combined_type(ast);
  } break;
  case AST_BIN_OP_MOD_ASSIGN:
  case AST_BIN_OP_SHL_ASSIGN:
  case AST_BIN_OP_SHR_ASSIGN:
  case AST_BIN_OP_AND_ASSIGN:
  case AST_BIN_OP_XOR_ASSIGN:
  case AST_BIN_OP_OR_ASSIGN: {
    if (left->variant != AST_IDENTIFIER ||
        find_symbol(current_scope, left->ident.name)->variant !=
            SYMBOL_VARIABLE) {
      error(ast->line, "Left hand side of an assignment is not a variable");
      ast->type = error_type;
      return;
    }

    if (left->type->variant == TYPE_FLOAT ||
        right->type->variant == TYPE_FLOAT) {
      error(ast->line, "Binary operation %.*s is not defined for type float",
            STR_FMT_UNWRAP(binop_str));
      ast->type = error_type;
      return;
    }

    make_combined_type(ast);
    ast->type = void_type;
  } break;
  default:
    break;
  }
}

void sema_unop(Ast *ast, Scope *current_scope, ModCType *return_type) {
  sema_check(ast->unary.expr, current_scope, return_type);

  switch (ast->unary.unop) {
  case AST_UNOP_ADDR_OF: {
    ast->type = make_pointer_type(ast->unary.expr->type, 1);
  } break;
  case AST_UNOP_DEREF: {
    if (ast->unary.expr->type->variant != TYPE_POINTER) {
      String *type_str = modctype_to_string(ast->unary.expr->type);
      error(ast->line, "Attempt to deference a non-pointer type %.*s",
            STR_FMT_UNWRAP(type_str));
      ast->type = error_type;
      return;
    }

    ast->type = ast->unary.expr->type->pointer.base;
  } break;
  case AST_UNOP_BIT_NOT:
  case AST_UNOP_MINUS: {
    if (ast->unary.expr->type->is_signed == false) {
      ast->type = signed_type_wider_than(ast->unary.expr->type->size);
    } else {
      ast->type = ast->unary.expr->type;
    }
  } break;
  case AST_UNOP_LOG_NOT: {
    ast->type = bool_type;
  } break;
  case AST_UNOP_POST_DEC:
  case AST_UNOP_PRE_DEC: {
    if (!t_is_numeric(ast->unary.expr->type)) {
      String *type_str = modctype_to_string(ast->unary.expr->type);
      error(ast->line, "Attempt to decrement non-numeric type %.*s",
            STR_FMT_UNWRAP(type_str));
      ast->type = error_type;
      return;
    }

    ast->type = ast->unary.expr->type;
  } break;
  case AST_UNOP_POST_INC:
  case AST_UNOP_PRE_INC: {
    if (!t_is_numeric(ast->unary.expr->type)) {
      String *type_str = modctype_to_string(ast->unary.expr->type);
      error(ast->line, "Attempt to increment non-numeric type %.*s",
            STR_FMT_UNWRAP(type_str));
      ast->type = error_type;
      return;
    }

    ast->type = ast->unary.expr->type;
  } break;
  default:
    ast->type = ast->unary.expr->type;
    break;
  }
}

void sema_funccall(Ast *ast, Scope *current_scope, ModCType *return_type) {
  Symbol *func = find_symbol(current_scope, ast->func_call.callee->ident.name);

  if (func == NULL) {
    error(ast->line, "Identifier \"%.*s\" used before declaration",
          STR_FMT_UNWRAP(ast->func_call.callee->ident.name));
    ast->type = error_type;
    return;
  }

  ast->type = func->type->function.return_type;

  Ast *first_kw_arg = NULL;

  for (size_t i = 0; i < ast->func_call.arg_count; i++) {
    b8 is_kw_arg = ast->func_call.args[i]->variant == AST_BINOP &&
                   ast->func_call.args[i]->binary.binop == AST_BIN_OP_ASSIGN;

    if (is_kw_arg && first_kw_arg == NULL)
      first_kw_arg = ast->func_call.args[i];

    if (!is_kw_arg && first_kw_arg != NULL) {
      error(ast->line, "Positional argument follows keyword argument");
      ast->type = error_type;
    }

    if (is_kw_arg) {
      sema_check(ast->func_call.args[i]->binary.right, current_scope,
                 return_type);
    } else {
      sema_check(ast->func_call.args[i], current_scope, return_type);
    }

    if (ast->func_call.args[i]->type == error_type)
      ast->type = error_type;
  }

  if (ast->type == error_type)
    return;

  ast->func_call.val_count = func->decl->func_decl.param_count;
  Ast **values = ast_create_array(ast->func_call.val_count);
  Ast **params = func->decl->func_decl.params;

  HashMap *param_name_to_idx = hashmap_new(&map_str_u64_type);

  for (size_t i = 0; i < func->decl->func_decl.param_count; i++) {
    Ast *param = params[i];

    hashmap_add(param_name_to_idx, (void *)param->var_decl.name, (void *)i);
  }

  for (size_t i = 0; i < ast->func_call.arg_count; i++) {
    Ast *arg = ast->func_call.args[i];

    b8 is_kw_arg =
        arg->variant == AST_BINOP && arg->binary.binop == AST_BIN_OP_ASSIGN;

    if (is_kw_arg) {
      if (!hashmap_has(param_name_to_idx, arg->binary.left->var_decl.name)) {
        error(arg->line, "%.*s() got an unexpected keyword argument \'%.*s\'",
              STR_FMT_UNWRAP(func->decl->func_decl.name),
              STR_FMT_UNWRAP(arg->binary.left->var_decl.name));
        ast->type = error_type;
        return;
      }

      u64 idx =
          (u64)hashmap_get(param_name_to_idx, arg->binary.left->var_decl.name);

      if (values[idx] != NULL) {
        error(arg->line,
              "%.*s() got multiple values for keyword argument \'%.*s\'",
              STR_FMT_UNWRAP(func->decl->func_decl.name),
              STR_FMT_UNWRAP(arg->binary.left->var_decl.name));
        ast->type = error_type;
        return;
      }
      values[idx] = arg->binary.right;
    } else {
      values[i] = arg;
    }
  }

  hashmap_release(param_name_to_idx);
  size_t num_missing = 0;
  Vec *missing = vec_new(&vec_str_type);

  for (size_t i = 0; i < func->decl->func_decl.param_count; i++) {
    if (values[i] == NULL) {
      if (params[i]->var_decl.initializer == NULL) {
        num_missing++;
        vec_push(missing, params[i]->var_decl.name);
      }
    }
  }

  if (num_missing > 0) {
    String *buf = str_new();
    str_catprintf(buf, "%.*s() missing %zu required positional arguments: ",
                  STR_FMT_UNWRAP(func->decl->func_decl.name), num_missing);

    for (size_t i = 0; i < missing->size; i++) {
      if (i != 0 && i < missing->size - 1) {
        str_cat_raw(buf, ", ", 2);
      } else if (i != 0) {
        str_cat_raw(buf, " and ", 5);
      }
      str_catprintf(buf, "\'%.*s\'",
                    STR_FMT_UNWRAP((String *)vec_get_at(missing, i)));
    }

    error(ast->line, buf->data);
    ast->type = error_type;

    vec_release(missing);

    return;
  }

  for (size_t i = 0; i < func->decl->func_decl.param_count; i++) {
    if (values[i] == NULL) {
      values[i] = params[i]->var_decl.initializer;
    }

    if (!types_can_fit(values[i]->type, params[i]->var_decl.type)) {
      String *val_type_str = modctype_to_string(values[i]->type);
      String *param_type_str = modctype_to_string(params[i]->var_decl.type);
      error(ast->line,
            "Cannot assign value of type %.*s to param \'%.*s\' of function "
            "%.*s() with type "
            "%.*s, an "
            "explicit cast may be required",
            STR_FMT_UNWRAP(val_type_str),
            STR_FMT_UNWRAP(params[i]->var_decl.name),
            STR_FMT_UNWRAP(func->decl->func_decl.name),
            STR_FMT_UNWRAP(param_type_str));
      ast->type = error_type;
    }
  }

  if (ast->type == error_type)
    return;

  ast->func_call.values = values;
}

void sema_check(Ast *ast, Scope *current_scope, ModCType *return_type) {
  switch (ast->variant) {
  case AST_BLOCK: {
    for (size_t i = 0; i < ast->block.stmt_count; i++) {
      if (ast->block.scope->variant == SCOPE_FUNCTION &&
          ast->block.statements[i]->variant == AST_FUNC) {
        error(ast->block.statements[i]->line,
              "Support for nested functions has not be implemented yet");
        ast->type = error_type;
        continue;
      }

      sema_check(ast->block.statements[i], ast->block.scope, return_type);
    }
  } break;
  case AST_FUNC: {
    Ast *first_kw_arg = NULL;

    for (size_t i = 0; i < ast->func_decl.param_count; i++) {
      if (ast->func_decl.params[i]->var_decl.initializer != NULL &&
          first_kw_arg == NULL) {
        first_kw_arg = ast->func_decl.params[i];
      }

      if (ast->func_decl.params[i]->var_decl.initializer == NULL &&
          first_kw_arg != NULL) {
        error(ast->line,
              "Parameter without default %.*s follows parameter with a default",
              STR_FMT_UNWRAP(ast->func_decl.params[i]->var_decl.name));
        ast->type = error_type;
      }

      sema_check(ast->func_decl.params[i], ast->func_decl.body->block.scope,
                 return_type);
    }

    sema_check(ast->func_decl.body, current_scope,
               ast->func_decl.func_type->function.return_type);

    Ast *last_stmt = ast->func_decl.body->block
                         .statements[ast->func_decl.body->block.stmt_count - 1];

    if (last_stmt->variant != AST_RETURN &&
        ast->func_decl.func_type->function.return_type != void_type) {
      error(last_stmt->line, "Non-void function does not return on all paths");
      ast->type = error_type;
    }
  } break;
  case AST_RETURN: {
    ast->type = void_type;

    if (ast->return_stmt.expr != NULL) {
      sema_check(ast->return_stmt.expr, current_scope, return_type);
      if (ast->return_stmt.expr->type == error_type) {
        ast->type = error_type;
        return;
      }

      ast->type = ast->return_stmt.expr->type;
    }

    if (!types_can_fit(ast->type, return_type)) {
      String *ret_type_str = modctype_to_string(return_type);
      String *ast_type_str = modctype_to_string(ast->type);
      error(ast->line,
            "Return type %.*s is does not match function return type %.*s, an "
            "explicit cast may be required",
            STR_FMT_UNWRAP(ast_type_str), STR_FMT_UNWRAP(ret_type_str));
      ast->type = error_type;
    }
  } break;
  case AST_CAST: {
    sema_check(ast->type_cast.expr, current_scope, return_type);
    ast->type = ast->type_cast.type;
  } break;
  case AST_IDENTIFIER: {
    Symbol *sym = find_symbol(current_scope, ast->ident.name);

    if (sym == NULL) {
      error(ast->line, "Identifier \"%.*s\" used before declaration",
            STR_FMT_UNWRAP(ast->ident.name));
      ast->type = error_type;
      return;
    }

    ast->type = sym->type;
  } break;
  case AST_BINOP: {
    sema_binop(ast, current_scope, return_type);
  } break;
  case AST_UNOP: {
    sema_unop(ast, current_scope, return_type);
  } break;
  case AST_VAR: {
    if (ast->var_decl.initializer == NULL) {
      ast->type = void_type;
      return;
    }

    sema_check(ast->var_decl.initializer, current_scope, return_type);

    String *var_type_str = modctype_to_string(ast->var_decl.type);
    String *init_type_str = modctype_to_string(ast->var_decl.initializer->type);

    if (ast->var_decl.type->variant !=
        ast->var_decl.initializer->type->variant) {
      error(ast->line,
            "Attempted to assign expression of type %.*s to variable %.*s with "
            "defined type %.*s",
            STR_FMT_UNWRAP(init_type_str), STR_FMT_UNWRAP(ast->var_decl.name),
            STR_FMT_UNWRAP(var_type_str));
      ast->type = error_type;
      return;
    }

    ast->type = void_type;
  } break;
  case AST_FUNCALL: {
    sema_funccall(ast, current_scope, return_type);
  } break;
  default:
    break;
  }
}
