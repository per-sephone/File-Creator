# File-Creator

File Creator is a multi-threaded program that accepts four command line parameters (d = location of directory, f = number of files to create, r = number of integers to insert into each output file and t = number of threads). It checks that d exists and creates f files in the directory d. It writes r random 32-bit integers into each file. Each of t threads creates approximately f/t of the files.

usage: ./filecreator <absolute path> <unsigned int> <unsigned int> <unsigned int>
