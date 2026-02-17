The bug exists in all recent GCC releases, and is worse in GCC-15.

- NOT A GCC BUG...This turns out to be an indeed violation of strict aliasing rule. See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124148 for details.

Below is an example with Debian stock GCC-14:

.. code-block:: sh

   $ gcc --version
   gcc (Debian 14.2.0-19) 14.2.0
   Copyright (C) 2024 Free Software Foundation, Inc.
   This is free software; see the source for copying conditions.  There is NO
   warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Good:

.. code-block:: sh

   $ gcc -O1 gcc_dse.c -o gcc_dse
   $ ./gcc_dse
   sizeof_A=32

Bad:

.. code-block:: sh

   $ gcc -O2 gcc_dse.c -o gcc_dse
   $ ./gcc_dse
   Segmentation fault

   $ valgrind ./gcc_dse
   ==3477936== Memcheck, a memory error detector
   ==3477936== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
   ==3477936== Using Valgrind-3.24.0 and LibVEX; rerun with -h for copyright info
   ==3477936== Command: ./gcc_dse
   ==3477936==
   ==3477936== Conditional jump or move depends on uninitialised value(s)
   ==3477936==    at 0x109189: sizeof_pnext (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x1091A4: sizeof_A.isra.0 (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x10905B: main (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==
   ==3477936== Conditional jump or move depends on uninitialised value(s)
   ==3477936==    at 0x10918E: sizeof_pnext (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x1091A4: sizeof_A.isra.0 (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x10905B: main (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==
   ==3477936== Use of uninitialised value of size 8
   ==3477936==    at 0x109180: sizeof_pnext (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x1091A4: sizeof_A.isra.0 (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x10905B: main (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==
   ==3477936== Invalid read of size 4
   ==3477936==    at 0x109180: sizeof_pnext (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x1091A4: sizeof_A.isra.0 (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x10905B: main (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==  Address 0x415641ff89495741 is not stack'd, malloc'd or (recently) free'd
   ==3477936==
   ==3477936==
   ==3477936== Process terminating with default action of signal 11 (SIGSEGV)
   ==3477936==  General Protection Fault
   ==3477936==    at 0x109180: sizeof_pnext (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x1091A4: sizeof_A.isra.0 (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ==3477936==    by 0x10905B: main (in /home/zzyiwei/gcc-dse-repro/gcc_dse)
   ...
   Segmentation fault


Mitigation 1: -fno-tree-dse

.. code-block:: sh

   $ gcc -O2 -fno-tree-dse gcc_dse.c -o gcc_dse
   $ ./gcc_dse
   sizeof_A=32

   $ valgrind ./gcc_dse
   ==3478122== Memcheck, a memory error detector
   ==3478122== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
   ==3478122== Using Valgrind-3.24.0 and LibVEX; rerun with -h for copyright info
   ==3478122== Command: ./gcc_dse
   ...
   sizeof_A=32

Mitigation 2: uncomment line 95: //__asm__ volatile("" : : "g"(a.pNext) : "memory");

.. code-block:: sh

   $ gcc -O2 gcc_dse.c -o gcc_dse
   $ ./gcc_dse
   sizeof_A=32

   $ valgrind ./gcc_dse
   ==3479238== Memcheck, a memory error detector
   ==3479238== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
   ==3479238== Using Valgrind-3.24.0 and LibVEX; rerun with -h for copyright info
   ==3479238== Command: ./gcc_dse
   ...
   sizeof_A=32

DSE dump: confirm with: Deleted dead store: a.pNext = &b;

.. code-block:: sh

   $ gcc -O2 -fdump-tree-dse-details gcc_dse.c -o gcc_dse
   $ grep "Deleted dead store:" -Hrn ./gcc_dse.c.*
   ./gcc_dse.c.044t.dse1:125:  Deleted dead store: a.dummy = 11;
   ./gcc_dse.c.044t.dse1:129:  Deleted dead store: a.sType = 7;
   ./gcc_dse.c.044t.dse1:133:  Deleted dead store: b.dummy = 47;
   ./gcc_dse.c.044t.dse1:137:  Deleted dead store: b.pNext = 0B;
   ./gcc_dse.c.044t.dse1:141:  Deleted dead store: b.sType = 13;
   ./gcc_dse.c.122t.dse2:76:  Deleted dead store: a.pNext = &b;

Mitigation 3: -fno-strict-aliasing

.. code-block:: sh

   $ gcc -O2 -fno-strict-aliasing gcc_dse.c -o gcc_dse
   $ ./gcc_dse
   sizeof_A=32
