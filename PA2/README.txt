Documentation for PA2
=====================

+------------------------+
| BUILD & RUN (Required) |
+------------------------+

Replace "(Comments?)" with either "32-bit standard Ubuntu 16.04" or "viterbi-scf1.usc.edu".
    The grader should grade my submission on: 32-bit standard Ubuntu 16.04

Replace "(Comments?)" with the command the grader should use to compile your code (should simply be "make pa2").
Please understand that the ONLY acceptable compiler is g++ and each compile command must begin with "g++ -g -Wall -std=c++11".
If you ask the grader to use something else, the grader will not be allowed to comply and you will get a zero for your assignment.
    To compile your code, the grader should type: make pa2

+-------------------------+
| SELF-GRADING (Required) |
+-------------------------+

Replace each "?" below with a numeric value:

(A) Run "pa2" as server, log to log file : 2 out of 2 pts
(B) Run "pa2" as server, log to log file, run "wget" as client : 11 out of 11 pts
(C) Run "pa2" as server, log to cout, run "wget" as client : 8 out of 8 pts
(D) Run "pa2" as server, log to cout, run "wget" as client, persistent connection : 29 out of 29 pts
(E) Run "pa2" as client using a persistent connection : 15 out of 15 pts
(F) Run "pa2" as client, response header has no "Content-Length" key : 5 out of 5 pts
(G) Run "pa2" as client to download a huge file : 11 out of 11 pts
(H) Run "pa2" client against "pa2" server : 19 out of 19 pts

Missing/incomplete required section(s) in README file : -0 pts
Submitted binary file : -0 pts
Cannot compile : -0 pts
"make clean" : -0 pts
Segmentation faults : -0 pts
Freezes : -0 pts
Web client program freezes (never terminates) : -0 pts
Require to move data or executable files : -0 pts
Require to use a different port number to grade : -0 pts

+---------------------------------+
| BUGS / TESTS TO SKIP (Required) |
+---------------------------------+

Are there are any tests mentioned in the grading guidelines test suite that you
know that it's not working and you don't want the grader to run it at all so you
won't get extra deductions, please replace "(Comments?)" below with your list.
(Of course, if the grader won't run such tests in the plus points section, you
will not get plus points for them; if the garder won't run such tests in the
minus points section, you will lose all the points there.)  If there's nothing
the grader should skip, please replace "(Comments?)" with "none".

Please skip the following tests: NONE

+-------------------------------------------------------------------------------------------+
| ADDITIONAL INFORMATION FOR GRADER (Optional, but the grader would read what you add here) |
+-------------------------------------------------------------------------------------------+

+-----------------------------------------------+
| OTHER (Optional) - Not considered for grading |
+-----------------------------------------------+

Comments on design decisions: NONE
