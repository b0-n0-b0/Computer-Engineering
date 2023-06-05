import matplotlib.pyplot as plt
from qiskit import IBMQ, Aer
from qiskit import QuantumCircuit, ClassicalRegister, QuantumRegister, execute
from qiskit.visualization import plot_histogram, plot_bloch_multivector
from math import pi
#Register creation
qr = QuantumRegister(2) #Quantum reg
cr = ClassicalRegister(2) #classic reg

exam = QuantumCircuit(qr, cr) #Circuit creation

exam.barrier() #Y1

# H and X gates
exam.x(qr[1])
exam.h(qr[0])
exam.barrier() #Y2

#CNot gate
exam.cx(qr[0], qr[1])
exam.barrier() #Y3

#Rx(pi/2) gate
exam.rx(pi/2,qr[1])
exam.barrier() #Y4

#S gate
exam.s(qr[0])
exam.barrier() #Y5

exam.measure(qr[0], cr[1])
exam.measure(qr[1], cr[0])

exam.draw(output='mpl')


shots = 2048

backend = Aer.get_backend('qasm_simulator') #get the backend
res_exe = execute(exam, backend=backend, shots=shots).result() #calculate the result
res = res_exe.get_counts() 
#print and plot the result
print(res)
plot_histogram(res)