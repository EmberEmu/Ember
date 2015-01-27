#🔥 **Ember Database Connection Pool**
---

#Basic overview
*Note: For brevity, `pool` is an alias of the namespace `ember::connection_pool`. This alias will be used in all code examples.*

The database connection pool is designed to be a lightweight, high-performance and database agnostic connection manager.

Here's a simple usage example:

```cpp
#include <pool/ConnectionPool.h>
#include <pool/Policies.h>
#include <pool/drivers/MySQL.h>
#include <chrono>

namespace sc = std::chrono;
namespace pool = ember::connection_pool;

void mysql_do_something(sql::Connection* connection) {
    // do something with the connection
}
    
int main() {
    ember::drivers::MySQL auth("root", "awfulpassword", "localhost", 3306, "login_db");
    
    pool::Pool<ember::drivers::MySQL, pool::CheckinClean, pool::ExponentialGrowth> pool
              (auth /* driver object */, 2 /* min. pool size */, 50 /* max. pool size */, seconds(300) /* connection keep-alive time */);
              
    pool::Connection<sql::Connection*> connection = pool.get_connection(); // use 'auto connection' for easier driver swapping
    mysql_do_something(connection()); // or connection.get();
    // rest of the program...
    // ...
} // all connections are released here - no need to perform any explicit closing on the pool

```

The two main components in this example are the Pool and MySQL 'driver' objects.

The driver implements basic functionality required for the pool to manage connections, such as the `open()` and `close()` member functions. 

The pool requires a handle to the driver object, the minimum number of connections to keep open and the maximum number of connections that the pool can open during growth. The template arguments indicate the driver type to use with the pool, a policy that dictates how the pool should behave when a connection is returned to the pool and a policy that dictates how the pool should handle growth.

Under the hood, the connection pool has a manager that runs in its own thread, periodically (default: 10 seconds) performing maintenance on the pool. The maintenance period can be set during the pool creation by passing an additional `std::chrono::seconds` argument as the final argument.

# Acquiring connections
There are several options for acquiring a connection from the pool.
```cpp
 pool::Connection<ConnectionType> connection = pool.get_connection();
```
The `get_connection()` member function always returns immediately. If a connection could not be acquired, it will throw an exception of type `ember::connection_pool::no_free_connections`. This exception will only be thrown if the pool has no available connections and is already at maximum capacity.

Here's a more complete usage example, including error handling:
```cpp
void foo() try {
    // create pool, etc
    pool::Connection<ConnectionType> connection = pool.get_connection();
} catch(pool::no_free_connections& e) {
    // pool has no available connections
} catch(sql::SQLException &e) {
    // handle MySQL failures
}
```

As can be seen in the example, the pool does not attempt to handle exceptions thrown from the driver (MySQL in this case). It's left up to the user to decide how they wish to proceed if a database error occurs.

Another option for acquiring connections allows the calling thread to wait for a connection to become available.
```cpp
pool::Connection<ConnectionType> connection = pool.wait_connection();
```

Alternatively:
```cpp
pool::Connection<ConnectionType> connection = pool.wait_connection(std::chrono::milliseconds(1000));
```

The first usage of `wait_connection()` will cause the caller to wait indefinitely for a connection to become available. It will never throw an exception of type `ember::connection::no_free_connections` but it will still propagate exceptions thrown from the database driver. 

The second example specifies how long the caller should wait for a connection before giving up. If the time elapses without being able to acquire a connection, `ember::connection_pool::no_free_connections` is thrown.

# Using connections
The pool does not restrict your usage of the connection. To acquire a handle to the connection, simply use the () operator or call .get(). Additionally, the -> operator can be used for transparent indirection.

For example, with a MySQL connection:
```cpp
void foo(sql::Connection* connection) { ... }

pool::Connection<sql::Connection*> connection = pool.get_connection();
foo(connection()); // fine
foo(connection.get()); // fine
connection->setAutoCommit(true); // MySQL Connector-C++ functionality, fine
```

# Sharing connections
One restriction on Connection objects is that they cannot be copied. For example:
```cpp
void foo(Connection<sql::Connection*> connection) { ... }

pool::Connection<sql::Connection*> connection = pool.get_connection();
foo(connection); // invalid
Connection<sql::Connection*> copy = connection; // invalid
```

You should think of a Connection object as similar to a `unique_ptr` in that it represents ownership of the connection it wraps.

A couple of possible alternatives:
```cpp
void foo(std::shared_ptr<Connection<sql::Connection*>> connection) { ... }

pool::Connection<sql::Connection*> connection = pool.get_connection();
foo(std::make_shared<Connection<sql::Connection*>>(connection)); // fine but not recommended
```

```cpp
void foo(Connection<sql::Connection*> connection) { ... }
void bar(sql::Connection* connection) { ... }

pool::Connection<sql::Connection*> connection = pool.get_connection();
foo(std::move(connection)); // fine, foo() now owns the connection
bar(connection.get()); // invalid! `connection` has been moved out!
```

# Releasing connections
Connections do not need to be explicitly released - RAII is used to ensure that it is safe to simply allow them to drop out of scope, thus preventing leaks. However, it is still possible to release a connection explicitly rather than hanging on to it for longer than necessary.

```cpp
pool::Connection<ConnectionType> connection = pool.get_connection();
// use connection
pool.release_connection(connection);
// do more work - do not use try to connection beyond this point
```

# Closing the pool
The pool can be closed implicitly by allowing it to go out of scope or explicitly by calling `close()`.

All connections taken from the pool must be returned before closing/destroying the pool. If there are still connections checked out upon explicitly calling `close()`, an exception of type of `ember::connection_pool::active_connections` will be thrown, containing the number of still checked out connections. Checked out connections will not be closed. It is possible to recover from this exception, as long as connections are checked back in before destruction, although it is bad form and not guaranteed to work in the future.

```cpp
auto connection = pool->get_connection();
pool->close(); // exception thrown
// handle exception
connection.close(); // or pool->return_connection(connection);
pool->reset(); // (delete, out of scope, etc) no crash but should still be fixed
```

If there are connections checked out when the pool's destructor is called, it will assert, crashing the application. Working with connections where the parent pool has been destroyed is almost guaranteed to crash the application, whether immediately or in the distant future (at application exit, for instance). Asserting in the pool makes it easier to find the source of the error, rather than having to deal with access violations at some unknown point in time.

```cpp
auto connection = pool->get_connection();
pool->reset(); // (delete, out of scope before connection, etc) assertion!
```