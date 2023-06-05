# importing Qiskit
from qiskit import QuantumCircuit, ClassicalRegister, QuantumRegister, execute
from qiskit import Aer

# import basic plot tools
from qiskit.visualization import plot_histogram

# Creating registers
qr = QuantumRegister(2)
cr = ClassicalRegister(2)

# Quantum circuit specification
exercise = QuantumCircuit(qr, cr)

# Insert the Hadamard gate on the first qr
exercise.h(qr[0])
exercise.barrier()

# Perform CNOT
exercise.cx(qr[0], qr[1])
exercise.barrier()

# Insert the X gate on the second qr
exercise.x(qr[1])
exercise.barrier()

# Measurement both qr
exercise.measure(qr[0], cr[0])
exercise.measure(qr[1], cr[1])

exercise.draw(output='mpl').savefig("circuit.pdf")
shots = 1024
# Let's see the results!
backend = Aer.get_backend('qasm_simulator')
exercise_results = execute(exercise, backend=backend, shots=shots).result()
answer = exercise_results.get_counts()
plot_histogram(answer).savefig("statistics.pdf")
