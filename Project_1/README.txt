Miro Garcia 
 
CIS 415 Operating Systems 

 
Project 1 Report Collection 

 
Submitted to: 
Prof. Allen Malony 

 
Author: 
Miro Nova Paris Garcia 
mirop 
951875906 

Miro Garcia 
Report 
Introduction 

I really enjoyed this project! This was certainly the hardest project 
I have done because we essentially start from scratch, so organization 
and understanding are crucial for success. This project was a mini terminal 
with a selection of built in commands and two modes. Interactive mode is
useful for everyday project building because you can interact with your 
files in real-time, while file mode is useful for if you already have the
commands you want to make your project run figured out ahead of time. 
The code behind them are basically the same, however both act as powerful 
tools for the user.

The commands themselves were fun to write because it is interesting to 
finally find out how theses commands worked. I laughed when I found out 
that mkdir just uses the mkdir function from the C library, which is built
into the OS itself. I also learned that error handling is harder to code 
that I thought, and I ended up making a separate file, error.c so I could 
easily refer back to all of them at once. I’m grateful for this project
because I believe even when it felt like I was wasting time while stuck 
on a silly error, I was learning how to be a more efficient programmer as well. 
 
Background 

What stands out the most in this project concerns the fact that it was less
helpful to use the high-level I/O functions such as printf() and fopen(). 
By using low-level system calls like write(), open(), dup2(), I noticed it 
gave me more control over how data flows between the shell and the OS. However
there was one point that frustrated me in file mode where it felt like I was 
forced to use fopen and fclose because the file that file mode takes in the 
command line, ./pseudo-shell –f input.txt, needs to be read line by line using
getline(), which requires a FILE* stream. Since getline() doesn’t work directly
using file descriptors returned by open(), I had to rely on fopen() despite 
wanting to keep the my whole project low-level. 

In file mode, the decision to use dup2() to redirect output to output.txt in
file mode, allowed the shell to behave like a real Unix shell when it takes
command output. I also found that using dynamic memory allocation with malloc()
was perfect for handling commands like ls which may have varying output lengths
depending on how big the workspace becomes. In addition, using getline() as
well which also uses malloc() helped me learn how much easier it is to understand
a project when you have complete control over every bit of memory.  

Implementation 

At first I tried to code execute_command in a way that did not require a 
ton of if/else statements, but quickly realized sometimes it is okay to write
straightforward repetitive code if it makes the program easier to write. 
Regardless of that, probably the most annoying time I had was making sure I
was using all of the correct methods. There were several times that I would 
look back at code I had implemented and realized it was with high-level methods
that would only cause issues later according to my friend Josiah. Luckily the
manpages helped make the process a little easier because I could look up all
the appropriate methods and get an in-depth understanding of them as well.  

I also had an issue towards the end because the example_output.txt file wasn’t 
correct. Essentially it shows both the command itself and the output of the command 
underneath it. At first I pandered to the idea of writing my file mode to produce 
exactly that but my friend explained that made no sense and that it was contradictory
with the test script which I realized as well. I decided to ignore the weird 
example_output.txt and just follow what the test_script.sh seemed to want. 

Performance Results and Discussion 

I was told by Grace McLewee that this section becomes more important later
for projects 2 and 3. However she also said that the core idea for project 
one was just the fact that I managed to write the project dynamically using
malloc and in the end we had no memory leaks. 

Conclusion 

While I am exhausted from implementing this project, I found that I became
much quicker with the terminal itself, which was something that held me back
when I first started CS 212. I almost refused to believe that using the 
terminal was faster than clicking with the mouse, but now I have realized 
just how convenient having a terminal to control my computer is.  

For some reason I enjoyed learning how deletFile works and how it needs unlink, 
a method I have never head of before to delete it out of memory and make the memory
reusable. I thought that was interesting because we’ve always been learning how 
to allocate memory but haven’t talked much about the process of deleting memory, 
which I would like to learn more about. Deleting something on a computer has always
seemed mysterious to me but now I know it is just about freeing up memory space 
again so it can be re-used. 
