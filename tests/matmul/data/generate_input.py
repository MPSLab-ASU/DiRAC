import random

A = []
B = []
C = []

for i in range(5):
    A.append([random.randrange(1,20) for k in range(5)])
    B.append([random.randrange(1,20) for k in range(5)])
    C.append([0 for k in range(5)])

for i in range(5):
    for j in range(5):
        for k in range(5):
            C[i][j] += A[i][k] * B[k][j]

print(A)
print(B)
print(C)

outFiles = {
    "op1.txt": A,
    "op2.txt": B,
    "golden_op3.txt": C
}

for outFileName, arr in outFiles.items():
    with open(outFileName, "w") as outFile:
        for line in arr:
            outFile.write(" ".join([str(item) for item in line])+"\n")