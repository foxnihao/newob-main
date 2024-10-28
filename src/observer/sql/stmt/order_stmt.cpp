#include "common/rc.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/stmt/order_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

OrderStmt::~OrderStmt()
{
  delete order_unit_;
}

RC OrderStmt::create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const OrderBySqlNode &order, OrderStmt *&stmt)
{
  RC rc = RC::SUCCESS;

  OrderUnit* order_unit = new OrderUnit();

  OrderStmt *tmp_stmt = new OrderStmt();

  Table *table = nullptr;
  const FieldMeta *field = nullptr;
  rc =get_table_and_field(db, default_table, tables, *order.field, table, field);
  if (rc != RC::SUCCESS) {
    //LOG_WARN("cannot find attr");
    return rc;
  }

  order_unit->set_field(Field(table, field));
  order_unit->set_type(order.order);

  tmp_stmt->set_unit(order_unit);

  stmt=tmp_stmt;

  return rc;
}