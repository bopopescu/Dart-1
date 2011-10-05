// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test program for testing throw statement

interface TestException {
  String getMessage();
}

class MyException implements TestException {
  const MyException([String message = ""]) : message_ = message;
  String getMessage() { return message_; }
  final String message_;
}

class MyException2 implements TestException {
  const MyException2([String message = ""]) : message_ = message;
  String getMessage() { return message_; }
  final String message_;
}

class MyException3 implements TestException {
  const MyException3([String message = ""]) : message_ = message;
  String getMessage() { return message_; }
  final String message_;
}

class Helper {
  static int f1(int i) {
    try {
      int j;
      j = func();
      if (j > 0) {
        throw new MyException2("Test for exception being thrown");
      }
    } catch (MyException3 exception) {
      i = 100;
      print(exception.getMessage());
    } catch (TestException exception) {
      i = 50;
      print(exception.getMessage());
    } catch (MyException2 exception) {
      i = 150;
      print(exception.getMessage());
    } catch (MyException exception) {
      i = 200;
      print(exception.getMessage());
    } finally {
      i = i + 800;
    }
    return i;
  }

  static int func() {
    int i = 0;
    while (i < 10) {
      i++;
    }
    return i;
  }
}

class Throw1Test {
  static testMain() {
    Expect.equals(850, Helper.f1(1));
  }
}

main() {
  Throw1Test.testMain();
}
