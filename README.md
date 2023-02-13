# Operating System Course Projects

## Project 1 : Socket Programming Auction
An auction between several buyers and sellers using linux system calls and sockets. This project uses UDP and TCP for communication between user terminals.

## Project 2 : Map Reduce Genre Counter
This project uses a MapReduce model to count the number of books in each genre and print out the result. The parallelization is done through creating processes. There are `counting_process` which counts number of books in each part file, and there is a `genre_processor` which gather the results of the later processes and sum up the number of books in each genre. The communication between processes are done with pipes.

## Project 3 : Parallel Image Filter
Four different filters are applied on a picture; To test the parallelization effect, same thing is first done in a serial manner an then in a parallel manner. The filters applied are `emboss_filter`, `horizontal_mirror_filter`, and `draw_rhombus`. The `Pthreads` library is used in this project.