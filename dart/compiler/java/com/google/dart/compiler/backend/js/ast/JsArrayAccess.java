// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.backend.js.ast;

/**
 * Represents a javascript expression for array access.
 */
public final class JsArrayAccess extends JsExpression {

  private JsExpression arrayExpr;
  private JsExpression indexExpr;

  public JsArrayAccess() {
    super();
  }

  public JsArrayAccess(JsExpression arrayExpr, JsExpression indexExpr) {
    this.arrayExpr = arrayExpr;
    this.indexExpr = indexExpr;
  }

  public JsExpression getArrayExpr() {
    return arrayExpr;
  }

  public JsExpression getIndexExpr() {
    return indexExpr;
  }

  @Override
  public boolean hasSideEffects() {
    return arrayExpr.hasSideEffects() || indexExpr.hasSideEffects();
  }

  @Override
  public boolean isDefinitelyNotNull() {
    return false;
  }

  @Override
  public boolean isDefinitelyNull() {
    return false;
  }

  public void setArrayExpr(JsExpression arrayExpr) {
    this.arrayExpr = arrayExpr;
  }

  public void setIndexExpr(JsExpression indexExpr) {
    this.indexExpr = indexExpr;
  }

  @Override
  public void traverse(JsVisitor v, JsContext ctx) {
    if (v.visit(this, ctx)) {
      arrayExpr = v.accept(arrayExpr);
      indexExpr = v.accept(indexExpr);
    }
    v.endVisit(this, ctx);
  }

  @Override
  public NodeKind getKind() {
    return NodeKind.ARRAY_ACCESS;
  }
}
