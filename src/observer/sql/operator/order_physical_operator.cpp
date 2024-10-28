#include "sql/operator/order_physical_operator.h"
#include "storage/table/table.h"
#include "event/sql_debug.h"
#include "sql/stmt/order_stmt.h"

std::vector<std::vector<Tuple *>> split_tuples(std::vector<Tuple *> tuples, const TupleCellSpec &tuple_schema);

RC OrderPhysicalOperator::open(Trx *trx) {
    if (children_.empty()) {
        return RC::SUCCESS;
    }

    // 递归调用open，递归调用的目的就是将trx传给每一个算子
    RC rc = children_[0]->open(trx);
    if (rc != RC::SUCCESS) {
        LOG_WARN("failed to open child operator: %s", strrc(rc));
        return rc;
    }
    
    rc = fetch_and_sort_table();
    return rc;
}

RC OrderPhysicalOperator::fetch_and_sort_table() {
    RC rc = RC::SUCCESS;
    // 循环从孩子节点获取数据
    PhysicalOperator *oper = children_.front().get();

    while (RC::SUCCESS == (rc = oper->next())) {
        Tuple* tuple = oper->current_tuple();
        LOG_WARN("not clone %s", tuple->to_string().c_str());
        if (tuple) {
          auto e=tuple->clone();
            tuples_.push_back(e); 
        }
    }

    // 输出调试信息
    for (auto &t: tuples_) {
        auto temp = t->to_string();
        LOG_WARN("%s", temp.c_str());
    }

    std::vector<std::vector<Tuple *>> tuples_list;
    tuples_list.push_back(tuples_);

    // 排序
    for (auto &order: orders_) {
        auto table_name = order->field().table()->name();
        auto field_name = order->field().field_name();
        auto tuple_schema = TupleCellSpec(table_name, field_name);
        auto order_type = order->type();

        std::vector<std::vector<Tuple *>> tmp;

        for (auto &tuples : tuples_list) {
            if (tuples.empty()) continue; // 避免对空容器排序

            std::sort(tuples.begin(), tuples.end(), [&](Tuple *left, Tuple *right) {
                Value left_value, right_value;
                if (left->find_cell(tuple_schema, left_value) != RC::SUCCESS ||
                    right->find_cell(tuple_schema, right_value) != RC::SUCCESS) {
                    return false; // 处理查找失败的情况
                }
                int res = left_value.compare(right_value);
                return (order_type == Order::ASC) ? (res < 0) : (res > 0);
            });

            auto splited_tuples = split_tuples(tuples, tuple_schema);
            tmp.insert(tmp.end(), splited_tuples.begin(), splited_tuples.end());
        }

        tuples_list = tmp;
    }

    // 合并结果
    std::vector<Tuple *> tmp_tuples;
    for (const auto &tuples : tuples_list) {
        tmp_tuples.insert(tmp_tuples.end(), tuples.begin(), tuples.end());
    }

    tuples_.swap(tmp_tuples);
    index_ = -1; // 重置索引
    return RC::SUCCESS;
}

std::vector<std::vector<Tuple *>> split_tuples(std::vector<Tuple *> tuples, const TupleCellSpec &tuple_schema) {
    Value last_value(AttrType::UNDEFINED);
    Value cur_value(AttrType::UNDEFINED);
    std::vector<Tuple*> cur_tuples;
    std::vector<std::vector<Tuple *>> tuples_list;

    for (auto tuple : tuples) {
        if (tuple->find_cell(tuple_schema, cur_value) != RC::SUCCESS) {
            continue; // 处理查找失败的情况
        }

        if (last_value.attr_type() == AttrType::UNDEFINED || last_value.compare(cur_value) == 0) {
            cur_tuples.push_back(tuple);
            last_value = cur_value;
        } else {
            tuples_list.push_back(cur_tuples);
            cur_tuples.clear(); // 清空当前元组集合
            cur_tuples.push_back(tuple);
            last_value = cur_value;
        }
    }

    if (!cur_tuples.empty()) {
        tuples_list.push_back(cur_tuples);
    }
    
    return tuples_list;
}

RC OrderPhysicalOperator::next() {
    if (index_ + 1 >= tuples_.size()) {
        return RC::RECORD_EOF;
    }
    index_++;
    return RC::SUCCESS;
}

RC OrderPhysicalOperator::close() {
    if (!children_.empty()) {
        children_[0]->close();
    }


    return RC::SUCCESS;
}

Tuple * OrderPhysicalOperator::current_tuple() {
    if (index_ < 0 || index_ >= tuples_.size()) {
        return nullptr; // 返回nullptr以避免无效访问
    }
    return tuples_[index_];
}
