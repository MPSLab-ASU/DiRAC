nRows = 1
nCols = 5

# 4-Tuple: <configVal, start_offset, cnt_keepVal, cnt_skipVal>
inputs = [
	{
		"id": 0,
		"rows": [
			[1, 0, 5, 0],
		],
		"cols": [
			[1, 0, 5, 0],
			[1, 0, 5, 0],
			[1, 0, 5, 0],
			[1, 0, 5, 0],
			[1, 0, 5, 0],
		]
	},
	{
		"id": 1,
		"rows": [
			[1, 0, 25, 0],
		],
		"cols": [
			[1, 0, 1, 4],
			[1, 1, 1, 4],
			[1, 2, 1, 4],
			[1, 3, 1, 4],
			[1, 4, 1, 4],
		]
	},
]

outputLines = []

for network in inputs:
	netId = network["id"]
	cols = network["cols"]
	rowTag, colTag = 0, 0

	def emitConfig(netId, ctrlId, ctrl, rowTag, colTag):
		output = []
		for j in range(4):
			line = "{}\t{}\t{}\t{}\t{}\t{}\n".format(
				netId,
				i,
				j,
				rowTag,
				colTag,
				ctrl[j]
			)
			output.append(line)
		return output

	for i, ctrl in enumerate(network["rows"]):
		output = emitConfig(netId, i, ctrl, i, 0)
		for line in output:
			outputLines.append(line)
	
	for i, ctrl in enumerate(network["cols"]):
		output = emitConfig(netId, i, ctrl, 0, i)
		for line in output:
			outputLines.append(line)

with open("config_network_controllers.txt", "w") as outFile:
	for line in outputLines:
		outFile.write(line)