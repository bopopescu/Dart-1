/*
 * Copyright 2011 Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.core.internal.builder;

import com.google.dart.compiler.DartArtifactProvider;
import com.google.dart.compiler.Source;

import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.Writer;
import java.net.URI;
import java.util.HashMap;
import java.util.Map;

/**
 * An in memory caching implementation of {@link DartArtifactProvider} that caches all content in
 * memory, never writing anything to disk.
 */
public abstract class CachingArtifactProvider extends DartArtifactProvider {
  private class CacheElement {
    String content;
    long lastModified;
  }

  /**
   * Answer the encoded URI scheme specific part so that we don't have to decode then encode when we
   * construct a URI. In other words, you can safely call {@link URI#create(String)} and
   * {@link URI#URI(String)} with the result of this method.
   */
  private static String getLocalUriPart(Source source, String part, String extension) {
    // even though the source uri uniquely identifies the source, we must use getName()
    // because the same source file can be referenced by two different libraries
    // and getName() returns a unique string which includes the library name
    StringBuilder builder = new StringBuilder(source.getName());
    if (part != null && part.length() > 0) {
      builder.append("$");
      builder.append(part);
    }
    builder.append(".");
    builder.append(extension);
    return builder.toString();
  }

  /**
   * A mapping of the value returned by {@link #getLocalUriPart(Source, String, String)} to artifact
   * content
   */
  private final Map<String, CacheElement> cache = new HashMap<String, CacheElement>();

  /**
   * Remove all artifacts from the cache
   */
  public void clearCachedArtifacts() {
    synchronized (cache) {
      cache.clear();
    }
  }

  /**
   * If content is cached locally, then return a reader for that content, otherwise return
   * <code>null</code>.
   */
  @Override
  public Reader getArtifactReader(Source source, String part, String extension) throws IOException {
    CacheElement elem;
    synchronized (cache) {
      elem = cache.get(getLocalUriPart(source, part, extension));
    }
    if (elem != null) {
      return new StringReader(elem.content);
    }
    return null;
  }

  @Override
  public URI getArtifactUri(Source source, String part, String extension) {
    return URI.create(getLocalUriPart(source, part, extension));
  }

  /**
   * Return a writer that will cache its contents locally in memory rather than wherever the parent
   * provider chooses (e.g. on disk).
   */
  @Override
  public Writer getArtifactWriter(Source source, String part, String extension) throws IOException {
    final String uriPart = getLocalUriPart(source, part, extension);
    return new StringWriter(4096) {
      @Override
      public void close() throws IOException {
        super.close();
        CacheElement elem = new CacheElement();
        elem.content = toString();
        elem.lastModified = System.currentTimeMillis();
        synchronized (cache) {
          cache.put(uriPart, elem);
        }
      }
    };
  }

  /**
   * Return <code>true</code> if the artifact is cached locally.
   * 
   * @param source the source file
   * @param base the base of the artifact to which the comparison is made
   * @param extension the artifact extension
   */
  @Override
  public boolean isOutOfDate(Source source, Source base, String extension) {
    synchronized (cache) {
      CacheElement elem = cache.get(getLocalUriPart(base, "", extension));
      return elem == null || elem.lastModified < source.getLastModified();
    }
  }
}
