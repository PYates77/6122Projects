# 6122Projects
School Projects

Currently only Project P2: DFT Threading
Tower.txt and Tower-Large.txt are the provided input files
after1d.txt shows the correct output for Tower.txt after a horizontal (rows only) pass of DFT
after2d.txt shows the correct final output for Tower.txt 

2D DFT is performed by doing a 1D DFT on each row of the array, then a 1D DFT on each column of the modified array
See DFT formula here: https://en.wikipedia.org/wiki/Discrete_Fourier_transform

Single threading functionality currently works as intended (set NUMTHREADS to 1), multithreading is bugged. 
