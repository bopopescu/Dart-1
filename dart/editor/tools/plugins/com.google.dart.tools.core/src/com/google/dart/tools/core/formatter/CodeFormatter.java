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
package com.google.dart.tools.core.formatter;

import com.google.dart.tools.core.internal.util.Util;

import org.eclipse.jface.text.IRegion;
import org.eclipse.text.edits.TextEdit;

/**
 * Instances of the class <code>CodeFormatter</code> define the behavior of objects that implement a
 * generic source code formatter.
 */
public abstract class CodeFormatter {
  /**
   * Unknown kind
   * <p>
   * If the corresponding comment options are set to <code>true</code> then it is also possible to
   * format the comments on the fly by adding the {@link #F_INCLUDE_COMMENTS} flag to this kind of
   * format.
   */
  public static final int K_UNKNOWN = 0x00;

  /**
   * Kind used to format an expression
   * <p>
   * Note that using this constant, the comments encountered while formatting the expression may be
   * shifted to match the correct indentation but are not formatted.
   * <p>
   * If the corresponding comment options are set to <code>true</code> then it is also possible to
   * format the comments on the fly by adding the {@link #F_INCLUDE_COMMENTS} flag to this kind of
   * format.
   */
  public static final int K_EXPRESSION = 0x01;

  /**
   * Kind used to format a set of statements
   * <p>
   * Note that using this constant, the comments encountered while formatting the statements may be
   * shifted to match the correct indentation but are not formatted.
   * <p>
   * If the corresponding comment options are set to <code>true</code> then it is also possible to
   * format the comments on the fly by adding the {@link #F_INCLUDE_COMMENTS} flag to this kind of
   * format.
   */
  public static final int K_STATEMENTS = 0x02;

  /**
   * Kind used to format a set of class body declarations
   * <p>
   * Note that using this constant, the comments encountered while formatting the body declarations
   * may be shifted to match the correct indentation but are not formatted.
   * <p>
   * If the corresponding comment options are set to <code>true</code> then it is also possible to
   * format the comments on the fly by adding the {@link #F_INCLUDE_COMMENTS} flag to this kind of
   * format.
   */
  public static final int K_CLASS_BODY_DECLARATIONS = 0x04;

  /**
   * Kind used to format a compilation unit
   * <p>
   * Note that using this constant, the comments are only indented while formatting the compilation
   * unit.
   * <p>
   * If the corresponding comment option is set to <code>true</code> then it is also possible to
   * format the comments on the fly by adding the {@link #F_INCLUDE_COMMENTS} flag to this kind of
   * format.
   */
  public static final int K_COMPILATION_UNIT = 0x08;

  /**
   * Kind used to format a single-line comment
   */
  public static final int K_SINGLE_LINE_COMMENT = 0x10;

  /**
   * Kind used to format a multi-line comment
   */
  public static final int K_MULTI_LINE_COMMENT = 0x20;

  /**
   * Kind used to format a Javadoc comment
   */
  public static final int K_JAVA_DOC = 0x40;

  /**
   * Flag used to include the comments during the formatting of the code snippet.
   * <p>
   * This flag can be combined with the following kinds:
   * <ul>
   * <li>{@link #K_COMPILATION_UNIT}</li>
   * <li>{@link #K_UNKNOWN}</li>
   * <li>{@link #K_CLASS_BODY_DECLARATIONS} <i>(since 3.6)</i></li>
   * <li>{@link #K_EXPRESSION} <i>(since 3.6)</i></li>
   * <li>{@link #K_STATEMENTS} <i>(since 3.6)</i></li>
   * </ul>
   * <p>
   * Note also that it has an effect only when one or several format comments options for
   * {@link DefaultCodeFormatterConstants#FORMATTER_COMMENT_FORMAT_JAVADOC_COMMENT javadoc} ,
   * {@link DefaultCodeFormatterConstants#FORMATTER_COMMENT_FORMAT_BLOCK_COMMENT block} or
   * {@link DefaultCodeFormatterConstants#FORMATTER_COMMENT_FORMAT_LINE_COMMENT line} are set to
   * {@link DefaultCodeFormatterConstants#TRUE true} while calling
   * {@link #format(int, String, int, int, int, String)} or
   * {@link #format(int, String, IRegion[], int, String)} methods
   * <p>
   * For example, with the Eclipse default formatter options, the formatting of the following code
   * snippet using {@link #K_COMPILATION_UNIT}:
   * 
   * <pre>
   * public class X {
   * &#047;&#042;&#042;
   *  &#042; This is just a simple example to show that comments will be formatted while processing a compilation unit only if the constant flag <code>F_INCLUDE_COMMENT</code> flag is set.
   *  &#042; &#064;param str The input string
   *  &#042;&#047;
   *  void foo(String str){}}
   * </pre>
   * will produce the following output:
   * 
   * <pre>
   * public class X {
   *  &#047;&#042;&#042;
   *     &#042; This is just a simple example to show that comments will be formatted while processing a compilation unit only if the constant flag <code>F_INCLUDE_COMMENT</code> flag is set.
   *     &#042;
   *     &#042; &#064;param str The input string
   *     &#042;&#047;
   *    void foo(String str){
   *    }
   *  }
   * </pre>
   * Adding this flavor to the kind given while formatting the same source (e.g.
   * {@link #K_COMPILATION_UNIT} | {@link #F_INCLUDE_COMMENTS}) will produce the following output
   * instead:
   * 
   * <pre>
   * public class X {
   *  &#047;&#042;&#042;
   *     &#042; This is just a simple example to show that comments will be formatted
   *     &#042; while processing a compilation unit only if the constant flag
   *     &#042; <code>F_INCLUDE_COMMENT</code> flag is set.
   *     &#042;
   *     &#042; &#064;param str
   *     &#042;     The input string
   *     &#042;&#047;
   *    void foo(String str){
   *    }
   *  }
   * </pre>
   */
  public static final int F_INCLUDE_COMMENTS = 0x1000;

  /**
   * Return the string that corresponds to the indentation to the given indentation level or an
   * empty string if the indentation cannot be computed.
   * <p>
   * This method needs to be overridden in a subclass.
   * <p>
   * The default implementation returns an empty string.
   * 
   * @param indentationLevel the given indentation level
   * @return the string corresponding to the right indentation level
   * @exception IllegalArgumentException if the given indentation level is lower than zero
   */
  public String createIndentationString(int indentationLevel) {
    return Util.EMPTY_STRING;
  }

  /**
   * Format <code>source</code>, and returns a text edit that correspond to the difference between
   * the given string and the formatted string.
   * <p>
   * It returns <code>null</code> if the given string cannot be formatted.
   * <p>
   * If the offset position is matching a whitespace, the result can include whitespaces. It would
   * be up to the caller to get rid of preceding whitespaces.
   * 
   * @param kind Use to specify the kind of the code snippet to format. It can be any of these:
   *          <ul>
   *          <li>{@link #K_EXPRESSION}</li> <li>{@link #K_STATEMENTS}</li> <li>
   *          {@link #K_CLASS_BODY_DECLARATIONS}</li> <li> {@link #K_COMPILATION_UNIT}<br>
   *          <b>Since 3.4</b>, the comments can be formatted on the fly while using this kind of
   *          code snippet<br>
   *          (see {@link #F_INCLUDE_COMMENTS} for more detailed explanation on this flag) </li> 
   *          <li>{@link #K_UNKNOWN}</li> <li> {@link #K_SINGLE_LINE_COMMENT}</li> <li>
   *          {@link #K_MULTI_LINE_COMMENT}</li> <li>{@link #K_JAVA_DOC}</li>
   *          </ul>
   * @param source the source to format
   * @param offset the given offset to start recording the edits (inclusive).
   * @param length the given length to stop recording the edits (exclusive).
   * @param indentationLevel the initial indentation level, used to shift left/right the entire
   *          source fragment. An initial indentation level of zero or below has no effect.
   * @param lineSeparator the line separator to use in formatted source, if set to <code>null</code>
   *          , then the platform default one will be used.
   * @return the text edit
   * @throws IllegalArgumentException if offset is lower than 0, length is lower than 0 or length is
   *           greater than source length.
   */
  public abstract TextEdit format(int kind, String source, int offset, int length,
      int indentationLevel, String lineSeparator);

  /**
   * Format <code>source</code>, and returns a text edit that correspond to the difference between
   * the given string and the formatted string.
   * <p>
   * It returns null if the given string cannot be formatted.
   * <p>
   * If an offset position is matching a whitespace, the result can include whitespaces. It would be
   * up to the caller to get rid of preceding whitespaces.
   * <p>
   * No region in <code>regions</code> must overlap with any other region in <code>regions</code>.
   * Each region must be within source. There must be at least one region. Regions must be sorted by
   * their offsets, smaller offset first.
   * 
   * @param kind Use to specify the kind of the code snippet to format. It can be any of these:
   *          <ul>
   *          <li>{@link #K_EXPRESSION}</li> <li>{@link #K_STATEMENTS}</li> <li>
   *          {@link #K_CLASS_BODY_DECLARATIONS}</li> <li> {@link #K_COMPILATION_UNIT}<br>
   *          <b>Since 3.4</b>, the comments can be formatted on the fly while using this kind of
   *          code snippet<br>
   *          (see {@link #F_INCLUDE_COMMENTS} for more detailed explanation on this flag) </li> 
   *          <li>{@link #K_UNKNOWN}</li> <li> {@link #K_SINGLE_LINE_COMMENT}</li> <li>
   *          {@link #K_MULTI_LINE_COMMENT}</li> <li>{@link #K_JAVA_DOC}</li>
   *          </ul>
   * @param source the source to format
   * @param regions a set of regions in source to format
   * @param indentationLevel the initial indentation level, used to shift left/right the entire
   *          source fragment. An initial indentation level of zero or below has no effect.
   * @param lineSeparator the line separator to use in formatted source, if set to <code>null</code>
   *          , then the platform default one will be used.
   * @return the text edit
   * @throws IllegalArgumentException if there is no region, a region overlaps with another region,
   *           or the regions are not sorted in the ascending order.
   */
  public abstract TextEdit format(int kind, String source, IRegion[] regions, int indentationLevel,
      String lineSeparator);
}