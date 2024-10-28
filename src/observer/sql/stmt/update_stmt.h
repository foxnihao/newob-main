class Table;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt 
{
public:
  ~UpdateStmt()override;
  UpdateStmt() = default;
  UpdateStmt(Table *table, const Value *values, int value_amount, FilterStmt *filter_stmt, const std::string& attribute_name);

  StmtType type() const override {
    return StmtType::UPDATE;
  }

public:
  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  Table *table() const
  {
    return table_;
  }
  const Value *values() const
  {
    return values_;
  }
  int value_amount() const
  {
    return value_amount_;
  }
  FilterStmt *filter_stmt() const {
    return filter_stmt_;
  }
  const std::string& attribute_name() const {
    return attribute_name_;
  }

private:
  Table *table_ = nullptr;
  const Value *values_ = nullptr;
  int value_amount_ = 0;
  FilterStmt *filter_stmt_ = nullptr;
  std::string attribute_name_;
};
