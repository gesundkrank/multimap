# C++ Reference

<br>
```cpp
#include "multimap/Map.hpp"
namespace multimap
```

## Map::Open

`void Open(`
<script>space(20)</script>`const boost::filesystem::path& directory,`
<script>space(20)</script>`const Options& options)`

Opens a map or creates a new one from/in `directory`.

Throws: `std::runtime_error` if something went wrong.

```cpp
multimap::Options options;
options.block_size = 512;
options.block_pool_memory = multimap::GiB(2);
options.create_if_missing = true;

multimap::Map map;
map.Open("/path/to/multimap", options);
```

## Map::Put

`void Put(const Bytes& key, const Bytes& value)`

Puts `value` into the list associated with `key`.

Throws: `std::runtime_error` if the value was too big. See [Limitations](index.md#limitations) for more details.

```cpp
map.Put("key", "value");
map.Put("key", std::to_string(12));

const auto pair = std::make_pair(34, 5.67);
map.Put("key", multimap::Bytes(&pair, sizeof pair));
// sizeof pair might be 16 due to alignment.
```

## Map::Get

`ConstIter Get(const Bytes& key) const`

Tries to acquire a read lock on the list associated with `key` and, if successful, returns a read-only iterator on it. If another thread already holds a write lock on the same list, the method blocks until the write lock is released. Multiple threads may acquire a read lock on the same list at the same time.

```cpp
auto iter = map.Get("key");
for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
  const multimap::Bytes value = iter.GetValue();
  DoSomething(value);
  // The value object actually points into an internal buffer
  // that may change during the iteration.  Therefore, if you
  // want to keep a copy of the value you can do so via
  // std::memcpy(dest_buf, value.data(), value.size())
  // or value.ToString().
}
```

## Map::GetMutable

`Iter GetMutable(const Bytes& key)`

Tries to acquire a write lock on the list associated with `key` and, if successful, returns an iterator that implements a `DeleteValue` operation. If another thread already holds a read or write lock on the same list, the method blocks until all locks are released.

```cpp
auto iter = map.GetMutable("key");
for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
  if (CanBeDeleted(iter.GetValue())) {
    iter.DeleteValue();
    // The value is now marked as deleted.  The operation is not reversible.
    // iter.HasValue() would yield false, so do not call iter.GetValue() again.
    // iter.Next() will advance the iterator to the next valid value.
  }
}
```

## Map::Contains

`bool Contains(const Bytes& key) const`

Checks if there is at least one value associated with `key`.


```cpp
const bool exists = map.Contains("key");
```

## Map::Delete

`std::size_t Delete(const Bytes& key)`

Deletes the entire list associated with `key`. If the list is currently locked, the method blocks until all locks are released.

Returns: The number of deleted values.

```cpp
const std::size_t num_deleted = map.Delete("key");
```

## Map::DeleteFirst

`bool DeleteFirst(const Bytes& key, Predicate predicate)`

Deletes the first value in the list associated with `key` for which `predicate` returns `true`.

Returns: `true` if a value was deleted, `false` otherwise.

```cpp
const auto predicate = [](const multimap::Bytes& value) {
  bool can_be_deleted = false;
  // Check value and set can_be_deleted = true to delete it.
  return can_be_deleted;
};
const bool success = map.DeleteFirst("key", predicate);
```

## Map::DeleteFirstEqual

`bool DeleteFirstEqual(const Bytes& key, const Bytes& value)`

Deletes all values in the list associated with `key` that are equal to `value` according to `operator==`.

Returns: `true` if a value was deleted, `false` otherwise.

```cpp
const bool success = map.DeleteFirstEqual("key", "pattern");
```

## Map::DeleteAll

`std::size_t DeleteAll(const Bytes& key, Predicate predicate)`

Deletes all values in the list associated with `key` for which `predicate` returns `true`.

Returns: The number of values deleted.

```cpp
const auto predicate = [](const multimap::Bytes& value) {
  bool can_be_deleted = false;
  // Check value and set can_be_deleted = true to delete it.
  return can_be_deleted;
};
const auto num_deleted = map.DeleteAll("key", predicate);
```

## Map::DeleteAllEqual

`std::size_t DeleteAllEqual(const Bytes& key, const Bytes& value)`

Deletes all values in the list associated with `key` that are equal to `value` according to `operator==`.

Returns: The number of values deleted.

```cpp
const auto num_deleted = map.DeleteAllEqual("key", "pattern");
```

## Map::ReplaceFirst

`bool ReplaceFirst(const Bytes& key, Function function)`

Replaces the first value in the list associated with `key` by the result of `function`, if any. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: `true` if a value was replaced, `false` otherwise.

```cpp
const auto replace = [](const multimap::Bytes& value) {
  std::string new_value;
  // Fill new_value with content or leave it empty.
  return new_value;
};
const auto success = map.ReplaceFirst("key", replace);
```

## Map::ReplaceFirstEqual

`bool ReplaceFirstEqual(`
<script>space(20)</script>`const Bytes& key,`
<script>space(20)</script>`const Bytes& old_value,`
<script>space(20)</script>`const Bytes& new_value)`

Replaces the first occurrence of `old_value` in the list associated with `key` by `new_value`. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: `true` if a value was replaced, `false` otherwise.

```cpp
const auto success = map.ReplaceFirstEqual("key", "old value", "new value");
```

## Map::ReplaceAll

`std::size_t ReplaceAll(const Bytes& key, Function function)`

Replaces all values in the list associated with `key` by the result of `function`, if any. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: The number of replaced values.

```cpp
const auto replace = [](const multimap::Bytes& value) {
  std::string new_value;
  // Fill new_value with content or leave it empty.
  return new_value;
};
const auto num_replaced = map.ReplaceAll("key", replace);
```

## Map::ReplaceAllEqual

`std::size_t ReplaceAllEqual(`
<script>space(20)</script>`const Bytes& key,`
<script>space(20)</script>`const Bytes& old_value,`
<script>space(20)</script>`const Bytes& new_value)`

Replaces all occurrences of `old_value` in the list associated with `key` by `new_value`. The replacements do not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: The number of replaced values.

```cpp
const auto num_replaced = map.ReplaceAllEqual("key", "old value", "new value");
```

## Map::ForEachKey

`void ForEachKey(Procedure procedure) const`

Applies `procedure` to each key of the map. The procedure must be a callable that is convertible or can be bound to an object of type [Procedure](#callablesprocedure). For the time of execution the entire map is locked for read-only operations, so you may call [Map::Get](#mapget) within `procedure`, but calling [Map::GetMutable](#mapgetmutable) would cause in a deadlock.

```cpp
const auto print = [&map](const multimap::Bytes& key) {
  std::cout << key.ToString() << " -> " << map.Get(key).NumValues() << '\n';
};
map.ForEachKey(print);
```

## Map::GetProperties

`std::map<std::string, std::string> GetProperties() const`

Collects and returns current properties of the map. This operation requires a scan of the entire internal table. For the time of execution the map is locked, so that all other operations will block.

```cpp
const auto properties = map.GetProperties();
for (const auto& entry : properties) {
  std::cout << entry.first << ": " << entry.second << '\n';
}
```

## Map::PrintProperties

`void PrintProperties() const`

Prints current properties of the map to `std::cout`. This operation requires a scan of the entire internal table. For the time of execution the map is locked, so that all other operations will block.

```cpp
map.PrintProperties();
```

## Map::ConstIter

An iterator type for read-only forward iteration. An iterator object owns a read lock on the underlying list. The lock is released automatically when the iterator gets destroyed. Iterators cannot be copied for single ownership reasons, but moving is allowed.

Apart from that, an iterator

- must always be initialized via `SeekToFirst` before iteration. This allows for lazy initialization of the interator, since `SeekToFirst` will cause a first disk access.
- has a `NumValues` method that returns the number of values, even if not yet initialized via `SeekToFirst`. If multiple iterators need to be processed, e.g. for intersection, `NumValues` should be used to select the shortest list.

```cpp
auto iter_a = map.Get("a");
auto iter_b = map.Get("b");
std::set<std::string> intersection;
if (iter_a.NumValues() < iter_b.NumValues()) {
  intersection = Intersect(iter_a, iter_b);
} else {
  intersection = Intersect(iter_b, iter_a);
}
```

## Map::Iter

Same as [Map::ConstIter](#mapconstiter), but acquires exclusive ownership of the underlying list. This iterator has a `DeleteValue` method that allows to mark values as deleted. Such values will be ignored on any subsequent iterations.

## Callables::Procedure

`typedef std::function<void(const Bytes&)> Procedure`

A callable type that processes an object of type `const Bytes&`. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from lambda or free functions functions.

## Callables::Predicate

`typedef std::function<bool(const Bytes&)> Predicate`

A callable type that processes an object of type `const Bytes&` and outputs a boolean value. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from lambda or free functions functions.

## Callables::Function

`typedef std::function<std::string(const Bytes&)> Function`

A callable type that processes an object of type `const Bytes&` and outputs an object of type `std::string`. The returned string is used as a raw memory wrapper in lieu of `Bytes`, because it manages its own memory so that the caller is not responsible for deallocation. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from lambda or free functions functions.