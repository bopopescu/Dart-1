/*
 * Copyright (c) 2011, the Dart project authors.
 *
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.debug.core.internal.sourceLookup;

import org.eclipse.debug.core.sourcelookup.AbstractSourceLookupDirector;
import org.eclipse.debug.core.sourcelookup.ISourceLookupParticipant;

/**
 * A collection of Dart specific source lookup participants.
 * 
 * @see org.eclipse.debug.core.sourcelookup.ISourceLookupParticipant
 */
public class DartSourceLookupDirector extends AbstractSourceLookupDirector {

  public DartSourceLookupDirector() {

  }

  @Override
  public void initializeParticipants() {
    addParticipants(new ISourceLookupParticipant[] {new DartSourceLookupParticipant()});
  }

}
