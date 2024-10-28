#include "sql/operator/update_physical_operator.h"
#include "storage/trx/trx.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, Value& value, const char * field_name) {
  table_ = table;
  value_ = value;

  if (field_name) {
            // 为 field_name_ 分配内存并复制字符串
            field_name_ = (char *)malloc(sizeof(char) * (strlen(field_name) + 1));
            if (field_name_) {
                strcpy(field_name_, field_name); // 复制字符串内容
            }
        }
  
}

UpdatePhysicalOperator::~UpdatePhysicalOperator() {
          if (field_name_ != nullptr) {
            free(field_name_); // 释放分配的内存
        }
}


RC UpdatePhysicalOperator::open(Trx *trx) {
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::close() {
    if (!children_.empty()) {
        children_[0]->close();
    }
    return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next() {
  RC rc = RC::SUCCESS;
  
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }

  PhysicalOperator *child = children_[0].get();
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();
    int index=0;
    const FieldMeta *field = table_->table_meta().field(field_name_, index);
    int offset = field->offset();
   //table_->update_record(record,field_name_,&value_);
   rc = trx_->update_record(table_, record, offset, index, value_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to delete record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::RECORD_EOF;
}