/*
 * Copyright (c) 2011, the Dart project authors.
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
package com.google.dart.indexer.index.entries;

import com.google.dart.indexer.locations.Location;

import org.eclipse.core.resources.IFile;

import java.util.Set;

public class DependentFileInfo implements DependentEntity {
  private final IFile file;

  public DependentFileInfo(IFile file) {
    if (file == null) {
      throw new NullPointerException("file is null");
    }
    this.file = file;
  }

  public IFile getFile() {
    return file;
  }

  @Override
  public boolean isStale(IFile staleFile, Set<Location> staleLocations) {
    return file.equals(staleFile);
  }

  @Override
  public String toString(boolean showLayers) {
    return "file " + file.getFullPath();
  }
}
