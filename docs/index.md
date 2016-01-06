<br>

Multimap is a fast 1:n key-value store that provides a mapping from keys to lists of values. It's about the same <a href="https://en.wikipedia.org/wiki/Multimap" target="_blank">data structure</a> you might already know from your very first computer science course, but beyond that it handles the external persistent storage of the entire data. Because Multimap is optimized for large numbers of n, it works perfectly as a building block for retrieval systems that employ <a href="https://en.wikipedia.org/wiki/Inverted_index" target="_blank">inverted indexing</a>.

<div class="row">
  <div class="col-md-6">
    <h2>Features</h2>
    <ul>
    <li>Embeddable store with a clean <a href="cppreference">C++</a> and <a href="javareference">Java interface</a>.</li>
    <li>Supported operations: put, get, remove, replace.</li>
    <li>Keys and values are arbitrary byte arrays.</li>
    <li>Keys are hold in memory, values are stored on disk.</li>
    <li>Import/export from/to Base64-encoded text files.</li>
    <li>Full thread-safe.</li>
    </ul>
  </div>
  <div class="col-md-6">
    <h2>Get Started</h2>
    <ul>
    <li>Read the <a href="overview/">overview</a> and learn the basics.</li>
    <li>Try the <a href="cpptutorial">C++</a> or <a href="javatutorial">Java tutorial</a> to get familiar with the API.</li>
    <br>
    <a class="btn btn-default btn-lg" href="downloadv03/" role="button"><span class="glyphicon glyphicon-download-alt" aria-hidden="true"></span>&nbsp;&nbsp;Download Multimap 0.3</a>
    </ul>
  </div>
</div>
<br>
<div class="row">
<div class="col-md-6">
<div class="panel panel-default">
<div class="panel-heading">C++ Example</div>
<div class="panel-body">
```cpp
#include <multimap/Map.hpp>

int main() {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map("path/to/directory", options);

  map.put("key", "1st value");
  map.put("key", "2nd value");

  auto iter = map.get("key");
  while (iter.hasNext()) {
    doSomething(iter.next());
  }
  
  // d'tor of iter releases the reader lock.
  // d'tor of map flushes in-memory data to disk. 
}
```
</div>
</div>
</div>
<div class="col-md-6">
<div class="panel panel-default">
<div class="panel-heading">Java Example</div>
<div class="panel-body">
```java
import io.multimap.*;

public static void main(String[] args) {
  Options options = new Options();
  options.setCreateIfMissing(true);
  Map map = new Map("path/to/directory", options);

  map.put("key", "1st value");
  map.put("key", "2nd value");

  Iterator iter = map.get("key");
  while (iter.hasNext()) {
    doSomething(iter.next());
  }
  
  iter.close();
  map.close();
}
```
</div>
</div>
</div>
</div>
<br>

Multimap is <a href="https://www.fsf.org/about/what-is-free-software" target="_bank">free software</a> implemented in standard C++11 and POSIX, distributed under the terms of the <a href="http://www.gnu.org/licenses/agpl-3.0.en.html" target="_blank">GNU Affero General Public License</a> (AGPL) version 3. At this time Multimap only runs under GNU/Linux on x86-32 and x86-64. This is also true for the included Java binding.