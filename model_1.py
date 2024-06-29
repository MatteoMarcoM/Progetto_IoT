import numpy as np
import tflite_runtime.interpreter as tfl
import matplotlib.pyplot as plt

# Load the TFLite model and allocate tensors.
interpreter = tfl.Interpreter(model_path="model_1.tflite")
interpreter.allocate_tensors()

# Get input and output tensors.
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Load test data
X_test = np.load('X_test_1.npy')
y_test = np.load('y_test_1.npy')

# Prepare the input data
input_shape = input_details[0]['shape']
output_shape = output_details[0]['shape']

#print(input_shape) # 1 40 1

#print(input_details) # 1 40 1

#print(X_test[0].shape) # 40 1 1

# Scatter plot per Actual vs Predicted Noise
y_test_plot = [] #np.zeros_like(y_test)
y_pred_plot = [] #np.zeros_like(y_test)

# cicle for each data point
for index, (x,y) in enumerate(zip(X_test, y_test)):

	# reshaping input data
	x = np.reshape(x.copy(), input_shape)
	y = np.reshape(y.copy(), output_shape)

	#print(x.shape) # 1 40 1
	#print(y.shape) # 1 40 1


	# Ensure input data has the correct shape
	#assert input_details[0]['shape'][1:] == input_shape[1:], "Shape of X_test does not match input shape of the model."

	# Set the input tensor
	interpreter.set_tensor(input_details[0]['index'], x.astype(np.float32))

	# Invoke the mode
	interpreter.invoke()

	# Get the output tensor
	y_pred_i = interpreter.get_tensor(output_details[0]['index'])

	print(str(index) + ") Predicted Noise: ", y_pred_i[0][0])

	# If you have a reference output for comparison
	print(str(index) + ") Actual Noise: ", y[0][0])

	# save current point
	y_test_plot.append(y)
	y_pred_plot.append(y_pred_i)
	
# Actual vs predicted plot
y_test_plot = np.array(y_test_plot)
y_pred_plot = np.array(y_pred_plot)

plt.figure(figsize=(14, 7))
plt.plot(y_test_plot.reshape(-1), color='blue', label='Actual Noise')
plt.plot(y_pred_plot.reshape(-1), color='red', label='Predicted Noise', linestyle='--')
plt.xlabel('N. Points')
plt.ylabel('Noise')
plt.title('Actual vs Predicted Noise')
plt.legend()
plt.grid()
plt.show()
