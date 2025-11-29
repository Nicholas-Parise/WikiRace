# ---------- Sequential ----------
#./pageRankSequential.exe > PR_Sequential/pageRank_Sequential.txt

# ---------- Shared Memory ----------
# pageRank_<threads>_<size>.txt

# fractional_problem_size=1
./pageRankSharedMemory.exe 1 1 > PR_Shared/pageRank_1_1.txt

./pageRankSharedMemory.exe 2 1 > PR_Shared/pageRank_2_1.txt

./pageRankSharedMemory.exe 4 1 > PR_Shared/pageRank_4_1.txt

./pageRankSharedMemory.exe 8 1 > PR_Shared/pageRank_8_1.txt

./pageRankSharedMemory.exe 16 1 > PR_Shared/pageRank_16_1.txt

# fractional_problem_size=0.5
./pageRankSharedMemory.exe 1 0.5 > PR_Shared/pageRank_1_0.5.txt

./pageRankSharedMemory.exe 2 0.5 > PR_Shared/pageRank_2_0.5.txt

./pageRankSharedMemory.exe 4 0.5 > PR_Shared/pageRank_4_0.5.txt

./pageRankSharedMemory.exe 8 0.5 > PR_Shared/pageRank_8_0.5.txt

./pageRankSharedMemory.exe 16 0.5 > PR_Shared/pageRank_16_0.5.txt

# fractional_problem_size=0.25
./pageRankSharedMemory.exe 1 0.25 > PR_Shared/pageRank_1_0.25.txt

./pageRankSharedMemory.exe 2 0.25 > PR_Shared/pageRank_2_0.25.txt

./pageRankSharedMemory.exe 4 0.25 > PR_Shared/pageRank_4_0.25.txt

./pageRankSharedMemory.exe 8 0.25 > PR_Shared/pageRank_8_0.25.txt

./pageRankSharedMemory.exe 16 0.25 > PR_Shared/pageRank_16_0.25.txt

# ---------- Distributed Memory ----------
# fractional_problem_size=1
mpiexec -n 1 .\pageRankDistributedMemory.exe 1 > PR_Distributed/pageRank_1_1.txt

mpiexec -n 2 .\pageRankDistributedMemory.exe 1 > PR_Distributed/pageRank_2_1.txt

mpiexec -n 4 .\pageRankDistributedMemory.exe 1 > PR_Distributed/pageRank_4_1.txt

# fractional_problem_size=0.5
mpiexec -n 1 .\pageRankDistributedMemory.exe 0.5 > PR_Distributed/pageRank_1_0.5.txt

mpiexec -n 2 .\pageRankDistributedMemory.exe 0.5 > PR_Distributed/pageRank_2_0.5.txt

mpiexec -n 4 .\pageRankDistributedMemory.exe 0.5 > PR_Distributed/pageRank_4_0.5.txt

# fractional_problem_size=0.25
mpiexec -n 1 .\pageRankDistributedMemory.exe 0.25 > PR_Distributed/pageRank_1_0.25.txt

mpiexec -n 2 .\pageRankDistributedMemory.exe 0.25 > PR_Distributed/pageRank_2_0.25.txt

mpiexec -n 4 .\pageRankDistributedMemory.exe 0.25 > PR_Distributed/pageRank_4_0.25.txt

