# project1
building a shell in c, we each had our own editing branches and would meet to consolidate these branches into our main. Our screenshots only show the commits to the main branch. 

Colin MacRae, Sophie Pavia, Sydney McGinnis

Our tar contains:

File: shellFinalProject.c
  our main program all in one file
 
Makefile: makefile
  our executiable is named shell
  
readme: README
  what you are reading now :)
  
screenschots of github commit log
  .png(s) will be tared with our files 

Division of Labor
  Part 1: Parsing () Everyone  
  
  Part 2: Env Var(): Sophie and Sydney
  
  Part 3: Prompt(): Sydney and Colin
  
  Part 4: Tilde Expansion(): Sophie and Sydney
  
  Part 5: $PATH Search(): Colin and Sydney
  
  Part 6: External Command Execution: Sophie and Colin
  
  Part 7: I/O Redirection(): Sophie and Sydney
  
  Part 8: Piping(): Sophie and Colin
  
  Part 9: Background Processing(): Colin and Sophie
  
  Part 10: Built in Functions(): Sydney and Colin


Bugs
  
  There is one bug with  "Print how long it took for the longest running command to execute." 
  It works for printing the longest process if it is solely a process. 
  Also if it is solely a background process. 
  finally if it is a regular process then a background process but not the other way around. To try
  to fix this bug we switched the order of the if/else statements and changed one else if statement in 
  main to a simple if statement. The bug has something to do with how the order of the code executes in
  main depending on it it checks for background processing time or regular processing time first.
