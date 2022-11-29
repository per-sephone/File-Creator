# File-Creator

File Creator is a multi-threaded program that accepts four command line parameters (d = location of directory, f = number of files to create, r = number of integers to insert into each output file and t = number of threads). It checks that d exists and creates f files in the directory d. It writes r random 32-bit integers into each file. Each of t threads creates approximately f/t of the files.

File Sorter reads each unsorted_<id>.bin file in directory D made by File Creator. It sort the integers found in the file and write the sorted integers to a new file <D>/sorted/sorted_<id>.bin

FileChecker checks the subset of files in <D>/sorted directory to make sure each file is correctly sorted version of the corresponding file in D using T threads.


filecreator.c usage: ./filecreator absolute-path  num-files  num-integers  num-threads

filesorter.c usage: ./filesorter  absolute-path

filechecker.c usage: ./filechecker  absolute-path  num-threads
