/*
Copyright (C) 2016- The University of Notre Dame
This software is distributed under the GNU General Public License.
See the file COPYING for details.
*/

#include "jx_eval.h"
#include "jx_print.h"
#include "jx_function.h"
#include "debug.h"

#include <string.h>

static struct jx * jx_eval_null( jx_operator_t op )
{
	struct jx *err;
	switch(op) {
		case JX_OP_EQ:
			return jx_boolean(1);
		case JX_OP_NE:
		case JX_OP_LT:
		case JX_OP_LE:
		case JX_OP_GT:
		case JX_OP_GE:
			return jx_boolean(0);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "unsupported operator on type null");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert_string(err, "op", jx_operator_string(op));
			return jx_error(err);
	}
}

static struct jx * jx_eval_boolean( jx_operator_t op, struct jx *left, struct jx *right )
{
	struct jx *err;
	int a = left ? left->u.boolean_value : 0;
	int b = right ? right->u.boolean_value : 0;

	switch(op) {
		case JX_OP_EQ:
			return jx_boolean(a==b);
		case JX_OP_NE:
			return jx_boolean(a!=b);
		case JX_OP_AND:
			return jx_boolean(a&&b);
		case JX_OP_OR:
			return jx_boolean(a||b);
		case JX_OP_NOT:
			return jx_boolean(!b);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "unsupported operator on type boolean");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
			return jx_error(err);
	}
}

static struct jx * jx_eval_integer( jx_operator_t op, struct jx *left, struct jx *right )
{
	struct jx *err;
	jx_int_t a = left ? left->u.integer_value : 0;
	jx_int_t b = right ? right->u.integer_value : 0;

	switch(op) {
		case JX_OP_EQ:
			return jx_boolean(a==b);
		case JX_OP_NE:
			return jx_boolean(a!=b);
		case JX_OP_LT:
			return jx_boolean(a<b);
		case JX_OP_LE:
			return jx_boolean(a<=b);
		case JX_OP_GT:
			return jx_boolean(a>b);
		case JX_OP_GE:
			return jx_boolean(a>=b);
		case JX_OP_OR:
			return jx_integer(a|b);
		case JX_OP_AND:
			return jx_integer(a&b);
		case JX_OP_ADD:
			return jx_integer(a+b);
		case JX_OP_SUB:
			return jx_integer(a-b);
		case JX_OP_MUL:
			return jx_integer(a*b);
		case JX_OP_DIV:
			if(b==0) {
				err = jx_object(NULL);
				jx_insert_string(err, "name", "RangeError");
				jx_insert_string(err, "message", "dividing by zero");
				jx_insert_string(err, "file", __FILE__);
				jx_insert_integer(err, "line", __LINE__);
				jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
				return jx_error(err);
			}
			return jx_integer(a/b);
		case JX_OP_MOD:
			if(b==0) {
				err = jx_object(NULL);
				jx_insert_string(err, "name", "RangeError");
				jx_insert_string(err, "message", "dividing by zero");
				jx_insert_string(err, "file", __FILE__);
				jx_insert_integer(err, "line", __LINE__);
				jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
				return jx_error(err);
			}
			return jx_integer(a%b);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "unsupported operator on type integer");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
			return jx_error(err);
	}
}

static struct jx * jx_eval_double( jx_operator_t op, struct jx *left, struct jx *right )
{
	struct jx *err;
	double a = left ? left->u.double_value : 0;
	double b = right ? right->u.double_value : 0;

	switch(op) {
		case JX_OP_EQ:
			return jx_boolean(a==b);
			break;
		case JX_OP_NE:
			return jx_boolean(a!=b);
			break;
		case JX_OP_LT:
			return jx_boolean(a<b);
			break;
		case JX_OP_LE:
			return jx_boolean(a<=b);
			break;
		case JX_OP_GT:
			return jx_boolean(a>b);
			break;
		case JX_OP_GE:
			return jx_boolean(a>=b);
			break;
		case JX_OP_ADD:
			return jx_double(a+b);
			break;
		case JX_OP_SUB:
			return jx_double(a-b);
			break;
		case JX_OP_MUL:
			return jx_double(a*b);
			break;
		case JX_OP_DIV:
			if(b==0) return jx_null();
			return jx_double(a/b);
		case JX_OP_MOD:
			if(b==0) return jx_null();
			return jx_double((jx_int_t)a%(jx_int_t)b);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "unsupported operator on type double");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
			return jx_error(err);
	}
}

static struct jx * jx_eval_string( jx_operator_t op, struct jx *left, struct jx *right )
{
	struct jx *err;
	const char *a = left ? left->u.string_value : "";
	const char *b = right ? right->u.string_value : "";

	switch(op) {
		case JX_OP_EQ:
			return jx_boolean(0==strcmp(a,b));
		case JX_OP_NE:
			return jx_boolean(0!=strcmp(a,b));
		case JX_OP_LT:
			return jx_boolean(strcmp(a,b)<0);
		case JX_OP_LE:
			return jx_boolean(strcmp(a,b)<=0);
		case JX_OP_GT:
			return jx_boolean(strcmp(a,b)>0);
		case JX_OP_GE:
			return jx_boolean(strcmp(a,b)>=0);
		case JX_OP_ADD:
			return jx_format("%s%s",a,b);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "unsupported operator on type string");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
			return jx_error(err);
	}
}

static struct jx * jx_eval_array( jx_operator_t op, struct jx *left, struct jx *right )
{
	struct jx *err;
	if (!(left && right)) return jx_null();

	switch(op) {
		case JX_OP_EQ:
			return jx_boolean(jx_equals(left, right));
		case JX_OP_ADD:
			return jx_array_concat(jx_copy(left), jx_copy(right), NULL);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "unsupported operator on type array");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(op, jx_copy(left), jx_copy(right)));
			return jx_error(err);
	}
}

/*
Handle a lookup operator, which has two valid cases:
1 - left is an object, right is a string, return the named item in the object.
2 - left is an array, right is an integer, return the nth item in the array.
*/

static struct jx * jx_eval_lookup( struct jx *left, struct jx *right )
{
	struct jx *err;
	if(left->type==JX_OBJECT && right->type==JX_STRING) {
		struct jx *r = jx_lookup(left,right->u.string_value);
		if(r) {
			return jx_copy(r);
		} else {
			err = jx_object(NULL);
			jx_insert_string(err, "name", "ReferenceError");
			jx_insert_string(err, "message", "key not found");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(JX_OP_LOOKUP, jx_copy(left), jx_copy(right)));
			return jx_error(err);
		}
	} else if(left->type==JX_ARRAY && right->type==JX_INTEGER) {
		struct jx_item *item = left->u.items;
		int count = right->u.integer_value;

		if(count<0) {
			err = jx_object(NULL);
			jx_insert_string(err, "name", "RangeError");
			jx_insert_string(err, "message", "index must be positive");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(JX_OP_LOOKUP, jx_copy(left), jx_copy(right)));
			return jx_error(err);
		}

		while(count>0) {
			if(!item) {
				err = jx_object(NULL);
				jx_insert_string(err, "name", "RangeError");
				jx_insert_string(err, "message", "index out of range");
				jx_insert_string(err, "file", __FILE__);
				jx_insert_integer(err, "line", __LINE__);
				jx_insert(err, jx_string("op"), jx_operator(JX_OP_LOOKUP, jx_copy(left), jx_copy(right)));
				return jx_error(err);
			}
			item = item->next;
			count--;
		}

		if(item) {
			return jx_copy(item->value);
		} else {
			err = jx_object(NULL);
			jx_insert_string(err, "name", "RangeError");
			jx_insert_string(err, "message", "index out of range");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(JX_OP_LOOKUP, jx_copy(left), jx_copy(right)));
			return jx_error(err);
		}
	} else {
		err = jx_object(NULL);
		jx_insert_string(err, "name", "TypeError");
		jx_insert_string(err, "message", "invalid type for lookup");
		jx_insert_string(err, "file", __FILE__);
		jx_insert_integer(err, "line", __LINE__);
		jx_insert(err, jx_string("op"), jx_operator(JX_OP_LOOKUP, jx_copy(left), jx_copy(right)));
		return jx_error(err);

	}
}

static struct jx *jx_eval_function( struct jx_function *f, struct jx *context )
{
	struct jx *err;

	if(!f) return NULL;
	switch(f->function) {
		case JX_FUNCTION_RANGE:
			return jx_function_range(f, context);
		case JX_FUNCTION_FOREACH:
			return jx_function_foreach(f, context);
		case JX_FUNCTION_STR:
			return jx_function_str(f, context);
		case JX_FUNCTION_JOIN:
			return jx_function_join(f, context);
		case JX_FUNCTION_DBG:
			return jx_function_dbg(f, context);
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "SyntaxError");
			jx_insert_string(err, "message", "invalid function");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("func"), jx_function(f->function, jx_copy(f->arguments)));
			return jx_error(err);
	}
}

/*
Type conversion rules:
Generally, operators are not meant to be applied to unequal types.
NULL is the result of an operator on two incompatible expressions.
Exception: When x and y are incompatible types, x==y returns FALSE and x!=y returns TRUE.
Exception: integers are promoted to doubles as needed.
Exception: The lookup operation can be "object[string]" or "array[integer]"
*/

static struct jx * jx_eval_operator( struct jx_operator *o, struct jx *context )
{
	if(!o) return 0;

	struct jx *left = jx_eval(o->left,context);
	struct jx *right = jx_eval(o->right,context);
	struct jx *err;
	struct jx *result;

	if (jx_istype(left, JX_ERROR)) {
		result = left;
		left = NULL;
		goto DONE;
	}
	if (jx_istype(right, JX_ERROR)) {
		result = right;
		right = NULL;
		goto DONE;
	}

	if((left && right) && (left->type!=right->type) ) {
		if( left->type==JX_INTEGER && right->type==JX_DOUBLE) {
			struct jx *n = jx_double(left->u.integer_value);
			jx_delete(left);
			left = n;
		} else if( left->type==JX_DOUBLE && right->type==JX_INTEGER) {
			struct jx *n = jx_double(right->u.integer_value);
			jx_delete(right);
			right = n;
		} else if(o->type==JX_OP_EQ) {
			jx_delete(left);
			jx_delete(right);
			return jx_boolean(0);
		} else if(o->type==JX_OP_NE) {
			jx_delete(left);
			jx_delete(right);
			return jx_boolean(1);
		} else if(o->type==JX_OP_LOOKUP) {
			struct jx *r = jx_eval_lookup(left,right);
			jx_delete(left);
			jx_delete(right);
			return r;
		} else {
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "mismatched types for operator");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(o->type, left, right));
			return jx_error(err);
		}
	}

	switch(right->type) {
		case JX_NULL:
			result = jx_eval_null(o->type);
			break;
		case JX_BOOLEAN:
			result = jx_eval_boolean(o->type,left,right);
			break;
		case JX_INTEGER:
			result = jx_eval_integer(o->type,left,right);
			break;
		case JX_DOUBLE:
			result = jx_eval_double(o->type,left,right);
			break;
		case JX_STRING:
			result = jx_eval_string(o->type,left,right);
			break;
		case JX_ARRAY:
			result = jx_eval_array(o->type,left,right);
			break;
		default:
			err = jx_object(NULL);
			jx_insert_string(err, "name", "TypeError");
			jx_insert_string(err, "message", "bad type of rvalue");
			jx_insert_string(err, "file", __FILE__);
			jx_insert_integer(err, "line", __LINE__);
			jx_insert(err, jx_string("op"), jx_operator(o->type, jx_copy(left), jx_copy(right)));
			result = jx_error(err);
			break;
	}

DONE:
	jx_delete(left);
	jx_delete(right);

	return result;
}

static struct jx_pair * jx_eval_pair( struct jx_pair *pair, struct jx *context )
{
	if(!pair) return 0;

	return jx_pair(
		jx_eval(pair->key,context),
		jx_eval(pair->value,context),
		jx_eval_pair(pair->next,context)
	);
}

static struct jx_item * jx_eval_item( struct jx_item *item, struct jx *context )
{
	if(!item) return 0;

	return jx_item(
		jx_eval(item->value,context),
		jx_eval_item(item->next,context)
	);
}

static struct jx *jx_check_errors(struct jx *j)
{
	struct jx *err = NULL;
	switch (j->type) {
		case JX_ARRAY:
			for(struct jx_item *i = j->u.items; i; i = i->next) {
				if(jx_istype(i->value, JX_ERROR)) {
					err = jx_copy(i->value);
					jx_delete(j);
					return err;
				}
			}
			return j;
		case JX_OBJECT:
			for(struct jx_pair *p = j->u.pairs; p; p = p->next) {
				if(jx_istype(p->key, JX_ERROR)) err = jx_copy(p->key);
				if(jx_istype(p->value, JX_ERROR)) err = jx_copy(p->value);
				if(err) {
					jx_delete(j);
					return err;
				}
			}
			return j;
		default:
			return j;
	}
}

struct jx * jx_eval( struct jx *j, struct jx *context )
{
	if(!j) return 0;

	switch(j->type) {
		case JX_SYMBOL:
			if(context) {
				struct jx *result = jx_lookup(context,j->u.symbol_name);
				if(result) {
					return jx_copy(result);
				} else {
					struct jx *err = jx_object(NULL);
					jx_insert_string(err, "name", "ReferenceError");
					jx_insert_string(err, "message", "undefined symbol");
					jx_insert_string(err, "file", __FILE__);
					jx_insert_integer(err, "line", __LINE__);
					jx_insert(err, jx_string("symbol"), jx_copy(j));
					jx_insert(err, jx_string("context"), jx_copy(context));
					return jx_error(err);
				}
			}
			return jx_null();
		case JX_DOUBLE:
		case JX_BOOLEAN:
		case JX_INTEGER:
		case JX_STRING:
		case JX_NULL:
			return jx_copy(j);
		case JX_ARRAY:
			return jx_check_errors(jx_array(jx_eval_item(j->u.items,context)));
		case JX_OBJECT:
			return jx_check_errors(jx_object(jx_eval_pair(j->u.pairs,context)));
		case JX_OPERATOR:
			return jx_eval_operator(&j->u.oper,context);
		case JX_FUNCTION:
			return jx_eval_function(&j->u.func,context);
		case JX_ERROR:
			return jx_copy(j);
	}
	/* not reachable, but some compilers complain. */
	return 0;
}
