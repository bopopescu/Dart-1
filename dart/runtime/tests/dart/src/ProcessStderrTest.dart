// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//
// Process test program to test process communication.

class ProcessStderrTest {

  static void testExit() {
    Process process = new Process("./process_test",
                                  const ["1", "1", "99", "0"]);
    final int BUFFERSIZE = 10;
    final int STARTCHAR = 65;
    Array<int> buffer = new Array<int>(BUFFERSIZE);
    for (int i = 0; (i < BUFFERSIZE - 1); i++) {
      buffer[i] = STARTCHAR + i;
    }
    buffer[BUFFERSIZE - 1] = 10;

    SocketInputStream input = process.stderrStream;
    SocketOutputStream output = process.stdinStream;

    process.start();

    Array<int> readBuffer = new Array<int>(BUFFERSIZE);

    void dataWritten() {
      void readData() {
        for (int i = 0; i < BUFFERSIZE; i++) {
          Expect.equals(buffer[i], readBuffer[i]);
        }
        process.close();
      }

      bool read = input.read(readBuffer, 0, BUFFERSIZE, readData); 
      if (read) {
        readData();
      }
    }
    bool written = output.write(buffer, 0, BUFFERSIZE, dataWritten);
    if (written) {
      dataWritten();
    }
  }

  static void testMain() {
    testExit();
  }
}

main() {
  ProcessStderrTest.testMain();
}