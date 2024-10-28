#pragma once

#include <vector>

#include "sql/expr/tuple.h"
#include "common/value.h"
#include "common/rc.h"

template <typename ExprPointerType>
class ExpressionTuple : public Tuple 
{
public:
ExpressionTuple(const std::vector<ExprPointerType> &expressions) : expressions_(expressions) {}


  RC spec_at(int index, TupleCellSpec &spec) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::INVALID_ARGUMENT;
    }

    const ExprPointerType &expression = expressions_[index];
    spec                              = TupleCellSpec(expression->name());
    return RC::SUCCESS;
  }
Tuple* clone() const override {
  return new ExpressionTuple(*this);
}

  virtual ~ExpressionTuple()
  {
  }

  int cell_num() const override
  {
    return expressions_.size();
  }

  RC cell_at(int index, Value &cell) const override
  {
    if (index < 0 || index >= cell_num()) {
      return RC::INVALID_ARGUMENT;
    }

    const ExprPointerType &expression = expressions_[index];
    return get_value(expression, cell);
  }
 void set_tuple(const Tuple *tuple) { child_tuple_ = tuple; }
  RC find_cell(const TupleCellSpec &spec, Value &cell) const override
  {
    for (auto &expr : expressions_) {
      if (0 == strcmp(spec.alias(), expr->name())) {
        return expr->try_get_value(cell);
      }
    }
    return RC::NOTFOUND;
  }
private:
  RC get_value(const ExprPointerType &expression, Value &value) const
  {
    RC rc = RC::SUCCESS;
    if (child_tuple_ != nullptr) {
      rc = expression->get_value(*child_tuple_, value);
    } else {
      rc = expression->try_get_value(value);
    }
    return rc;
  }

private:
   const std::vector<ExprPointerType> &expressions_;
  const Tuple                        *child_tuple_ = nullptr;
};