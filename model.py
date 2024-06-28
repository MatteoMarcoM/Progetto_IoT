import numpy as np
import tensorflow as tf

# Load the TFLite model and allocate tensors.
interpreter = tf.lite.Interpreter(model_path="converted_model.tflite")
interpreter.allocate_tensors()

# Get input and output tensors.
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Load test data
X_test = np.load('X_test.npy')
y_test = np.load('y_test.npy')

# Prepare the input data
input_shape = input_details[0]['shape']

# Ensure input data has the correct shape
assert X_test.shape[1:] == input_shape[1:], "Shape of X_test does not match input shape of the model."

# Set the input tensor
interpreter.set_tensor(input_details[0]['index'], X_test)

# Invoke the model
interpreter.invoke()

# Get the output tensor
y_pred = interpreter.get_tensor(output_details[0]['index'])

print("Predicted Noise:", y_pred)

# If you have a reference output for comparison
print("Actual Noise:", y_test)

# Scatter plot per Actual vs Predicted Noise
import matplotlib.pyplot as plt

plt.figure(figsize=(7, 7))
plt.scatter(y_test, y_pred, color='blue', label='Predicted vs Actual')
plt.plot([y_test.min(), y_test.max()], [y_test.min(), y_test.max()], 'k--', lw=2)
plt.xlabel('Actual Noise')
plt.ylabel('Predicted Noise')
plt.title('Scatter Plot: Actual vs Predicted Noise')
plt.legend()
plt.show()
