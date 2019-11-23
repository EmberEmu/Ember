# 🔥 **Ember DB Utils**
---

## Overview

`dbutils` is a simple tool to help with the initial database population and application of migration/update patches. Its primary usecase is for setting Ember up as part of Docker development deployments.

It's written in C++ rather than a scripting language to avoid being platform-specific or requiring yet another dependency.

## Usage

A usage example for a clean installation:

```bash
dbutils
    --install=login world \
    --login.root-user=root \
    --login.root-password=root \
    --login.set-user=ember_login \
    --login.set-password=changeme \
    --login.db-name=ember_login \
    --world.root-user=root \
    --world.root-password=root \
    --world.set-user=ember_world \
    --world.set-password=changeme \
    --world.db-name=ember_world \
    --sql-dir=sql/
```

When running with `--install`, the tool assumes that it must create the databases and users from scratch, thereby requiring a user with permissions to create the users specified by `-db-login_user` and `-db-world-user`.

A usage example for updating an existing database:

```bash
dbutils
    --update=login world \
    --login.root-user=root \
    --login.root-password=root \
    --login.db-name=ember_login \
    --world.root-user=root \
    --world.root-password=root \
    --world.db-name=ember_world \
    --sql-dir=sql/
```

When running with `--update`, the tool assumes that all databases have been populated and will attempt to locate and apply any patches required to bring them up to date.

Only specifying one database (e.g. world or login) will cause only that database to be updated.

Some additional arguments that you may find useful:

* `--<login/world>.hostname`, allows you to specify the hostname/address of the machine running the world or login database. Default is assumed to be localhost.
* `--<world/login>.port`, allows you to specify the port of world or login database. Default is assumed to be 3306.
* `--transactional-updates`, controls whether SQL updates are applied as a transactions. This allows the update to be rolled back upon failure, leaving the database in its origin state. The default is false. Note that updates to your schema (DDLs) generally cannot be rolled back.
* `--single-transaction`, controls whether to apply all updates in a single transaction rather than a transaction per update. This option takes precedence over `--transaction-updates`. The default is false. Note that updates to your schema (DDLs) generally cannot be rolled back.
* `--shutup`, skips the warning shown when running in update or clean install mode. Default is false.
* `--clean`, drops any existing users and databases that match the arguments given to --install.

Running `dbutils` with the `-h` argument will bring up a list of all available arguments and a short description of their functionality.

## Precautions

**To prevent accidental data loss, it is strongly recommended that you ensure your databases are backed up before running this tool.**

By default, the tool runs in batched transactional mode, meaning it will attempt to undo any and all changes in the event of a failed update. It also links the updates of any specified databases together, meaning that a failure to update one database will trigger a rollback on the updates to the others.

## Limitations

At present, the tool only supports MySQL/MariaDB.
