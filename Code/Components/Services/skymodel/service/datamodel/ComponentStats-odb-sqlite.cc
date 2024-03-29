// This file was generated by ODB, object-relational mapping (ORM)
// compiler for C++.
//

#include <odb/pre.hxx>

#define ODB_SQLITE_QUERY_COLUMNS_DEF
#include "ComponentStats-odb-sqlite.h"
#undef ODB_SQLITE_QUERY_COLUMNS_DEF

#include <cassert>
#include <cstring>  // std::memcpy

#include <odb/schema-catalog-impl.hxx>
#include <odb/function-table.hxx>

#include <odb/sqlite/traits.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/statement.hxx>
#include <odb/sqlite/statement-cache.hxx>
#include <odb/sqlite/view-statements.hxx>
#include <odb/sqlite/container-statements.hxx>
#include <odb/sqlite/exceptions.hxx>
#include <odb/sqlite/view-result.hxx>

namespace odb
{
  // ComponentStats
  //

  bool access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::
  grow (image_type& i,
        bool* t)
  {
    ODB_POTENTIALLY_UNUSED (i);
    ODB_POTENTIALLY_UNUSED (t);

    bool grew (false);

    // count
    //
    t[0UL] = false;

    return grew;
  }

  void access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::
  bind (sqlite::bind* b,
        image_type& i)
  {
    using namespace sqlite;

    sqlite::statement_kind sk (statement_select);
    ODB_POTENTIALLY_UNUSED (sk);

    std::size_t n (0);

    // count
    //
    b[n].type = sqlite::bind::integer;
    b[n].buffer = &i.count_value;
    b[n].is_null = &i.count_null;
    n++;
  }

  void access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::
  init (view_type& o,
        const image_type& i,
        database* db)
  {
    ODB_POTENTIALLY_UNUSED (o);
    ODB_POTENTIALLY_UNUSED (i);
    ODB_POTENTIALLY_UNUSED (db);

    // count
    //
    {
      ::std::size_t& v =
        o.count;

      sqlite::value_traits<
          ::std::size_t,
          sqlite::id_integer >::set_value (
        v,
        i.count_value,
        i.count_null);
    }
  }

  access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::query_base_type
  access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::
  query_statement (const query_base_type& q)
  {
    query_base_type r (
      "SELECT "
      "count(\"ContinuumComponent\".\"continuum_component_id\") ");

    r += "FROM \"ContinuumComponent\"";

    if (!q.empty ())
    {
      r += " ";
      r += q.clause_prefix ();
      r += q;
    }

    return r;
  }

  result< access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::view_type >
  access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::
  query (database&, const query_base_type& q)
  {
    using namespace sqlite;
    using odb::details::shared;
    using odb::details::shared_ptr;

    sqlite::connection& conn (
      sqlite::transaction::current ().connection ());
    statements_type& sts (
      conn.statement_cache ().find_view<view_type> ());

    image_type& im (sts.image ());
    binding& imb (sts.image_binding ());

    if (im.version != sts.image_version () || imb.version == 0)
    {
      bind (imb.bind, im);
      sts.image_version (im.version);
      imb.version++;
    }

    const query_base_type& qs (query_statement (q));
    qs.init_parameters ();
    shared_ptr<select_statement> st (
      new (shared) select_statement (
        conn,
        qs.clause (),
        false,
        true,
        qs.parameters_binding (),
        imb));

    st->execute ();

    shared_ptr< odb::view_result_impl<view_type> > r (
      new (shared) sqlite::view_result_impl<view_type> (
        qs, st, sts, 0));

    return result<view_type> (r);
  }

  result< access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::view_type >
  access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::
  query (database& db, const odb::query_base& q)
  {
    return query (db, query_base_type (q));
  }

  static const
  access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_common >::
  function_table_type function_table_askap_cp_sms_datamodel_ComponentStats_ =
  {
    &access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >::query
  };

  static const view_function_table_entry< ::askap::cp::sms::datamodel::ComponentStats, id_sqlite >
  function_table_entry_askap_cp_sms_datamodel_ComponentStats_ (
    &function_table_askap_cp_sms_datamodel_ComponentStats_);
}

namespace odb
{
  static bool
  create_schema (database& db, unsigned short pass, bool drop)
  {
    ODB_POTENTIALLY_UNUSED (db);
    ODB_POTENTIALLY_UNUSED (pass);
    ODB_POTENTIALLY_UNUSED (drop);

    if (drop)
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("CREATE TABLE IF NOT EXISTS \"schema_version\" (\n"
                      "  \"name\" TEXT NOT NULL PRIMARY KEY,\n"
                      "  \"version\" INTEGER NOT NULL,\n"
                      "  \"migration\" INTEGER NOT NULL)");
          db.execute ("DELETE FROM \"schema_version\"\n"
                      "  WHERE \"name\" = ''");
          return false;
        }
      }
    }
    else
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("CREATE TABLE IF NOT EXISTS \"schema_version\" (\n"
                      "  \"name\" TEXT NOT NULL PRIMARY KEY,\n"
                      "  \"version\" INTEGER NOT NULL,\n"
                      "  \"migration\" INTEGER NOT NULL)");
          db.execute ("INSERT OR IGNORE INTO \"schema_version\" (\n"
                      "  \"name\", \"version\", \"migration\")\n"
                      "  VALUES ('', 2, 0)");
          return false;
        }
      }
    }

    return false;
  }

  static const schema_catalog_create_entry
  create_schema_entry_ (
    id_sqlite,
    "",
    &create_schema);

  static const schema_catalog_migrate_entry
  migrate_schema_entry_1_ (
    id_sqlite,
    "",
    1ULL,
    0);

  static bool
  migrate_schema_2 (database& db, unsigned short pass, bool pre)
  {
    ODB_POTENTIALLY_UNUSED (db);
    ODB_POTENTIALLY_UNUSED (pass);
    ODB_POTENTIALLY_UNUSED (pre);

    if (pre)
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("UPDATE \"schema_version\"\n"
                      "  SET \"version\" = 2, \"migration\" = 1\n"
                      "  WHERE \"name\" = ''");
          return false;
        }
      }
    }
    else
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("UPDATE \"schema_version\"\n"
                      "  SET \"migration\" = 0\n"
                      "  WHERE \"name\" = ''");
          return false;
        }
      }
    }

    return false;
  }

  static const schema_catalog_migrate_entry
  migrate_schema_entry_2_ (
    id_sqlite,
    "",
    2ULL,
    &migrate_schema_2);
}

#include <odb/post.hxx>
