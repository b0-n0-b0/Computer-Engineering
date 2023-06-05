# initialization
import matplotlib.pyplot as plt

# importing Qiskit
from qiskit import IBMQ, Aer
from qiskit import QuantumCircuit, ClassicalRegister, QuantumRegister, execute
from qiskit import IBMQ, Aer

# import basic plot tools
from qiskit.visualization import plot_histogram, plot_bloch_multivector
from math import pi

# Creating registers
qr = QuantumRegister(2)
cr = ClassicalRegister(2)

# Quantum circuit specification
exercise = QuantumCircuit(qr, cr)

# Insert the S and X gates on the first qr
exercise.s(qr[0])
exercise.barrier()

exercise.x(qr[0])
exercise.barrier()

# Perform Controlled Rx gate
exercise.crx(pi, qr[0], qr[1])
exercise.barrier()

# Insert a CNOT gate where the control and target are swapped
exercise.cx(qr[1], qr[0])
exercise.barrier()

# Measurement both qr
exercise.measure(qr[0], cr[1])
exercise.measure(qr[1], cr[0])

exercise.draw(output='mpl').savefig("circuit.pdf")

shots = 1024
# Let's see the results!
backend = Aer.get_backend('qasm_simulator')
exercise_results = execute(exercise, backend=backend, shots=shots).result()
answer = exercise_results.get_counts()
plot_histogram(answer).savefig("statistics.pdf")
