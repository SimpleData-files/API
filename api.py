# This API assumes that the file\s accessed have been parsed by
# the parser with no errors
# Use of this API on a file with possible or confirmed errors is UB
# ------------------
# Written in Python3

import os
import subprocess

SIMPLEDATA_API_VERSION = "1.0"

# Functions to remove leading and trailing whitespace
def simpledata_remove_leading(string) -> str:
	start = 0
	buffer = ""

	for i in range(len(string)):
		if string[start] != ' ' and string[start] != '\t':
			break
		start += 1

	i = start
	while i < len(string):
		buffer += string[i]
		i += 1

	return buffer

def simpledata_remove_trailing(string) -> str:
	trailing = True
	buffer = ""

	i = len(string) - 1
	while i >= 0:
		if string[i] != ' ' and string[i] != '\t':
			trailing = False

		if trailing == False:
			buffer += string[i]

		i -= 1

	# Reverse the buffer (making it the right way around)
	rev = ""

	i = len(buffer) - 1
	while i >= 0:
		rev += buffer[i]		
		i -= 1

	return rev

errs = [
	[1, "Invalid File"],
	[2, "Identifier Not Found"],
	[3, "Invalid Type"]
]

class simpledata():
	file_name = "";
	err_code = 0

	def open(this, filename) -> int:
		try:
			file = open(filename, "r")
		except FileNotFoundError:
			this.err_code = 1

		file.close()
		this.err_code = 0

		this.file_name = filename
		return this.err_code

	def __init__(this, filename = ""):
		if filename != "":
			this.open(filename)

	def errstr(this) -> str:
		i = 0
		while i < len(errs):
			if errs[i][0] == this.err_code:
				return errs[i][1]

			i += 1
		return "Not An Error"

	# The type is the format in which the value wants to be outputted e.g., bool, string, int, float
	def fetch(this, identifier, type):

		# Only perform anything if the open file is valid
		if this.err_code != 1:
			file = open(this.file_name, "r");
			found = False

			# Only outside of the for loop so it can be returned after error code processing
			val_buffer = ""

			for line in file:
				if line != "" and line != "\n" and line[0] != '#':
					current_id = ""
					id_end = 0

					# Getting the identifier
					while id_end < len(line) and line[id_end] != ':':
						current_id += line[id_end];
						id_end += 1

					current_id = simpledata_remove_leading(current_id)
					current_id = simpledata_remove_trailing(current_id);

					if identifier != current_id:
						continue
					else:
						found = True

					# Getting the value
					i = id_end + 1
					while i < len(line):
						val_buffer += line[i]
						i += 1

					val_buffer = simpledata_remove_leading(val_buffer)
					val_buffer = simpledata_remove_trailing(val_buffer)
					break

			file.close()
			if found == True:
				this.err_code = 0
			else:
				this.err_code = 2

			# Converting the value to type specified by caller
			try:
				if type == "int":
					return int(val_buffer)
				elif type == "float":
					return float(val_buffer)
				elif type == "bool":
					if val_buffer == "true":
						return True
					elif val_buffer == "false":
						return False
					else:
						this.err_code = 3
						return None
				elif type == "char":
					return val_buffer[1]
				else:
					i = 1
					str_buffer = ""

					while i < len(val_buffer) - 1:
						str_buffer += val_buffer[i]
						i += 1

					return str_buffer
			except ValueError:
				this.err_code = 3
				return None

	def update(this, identifier, new_val, val_type) -> int:

		# Only do anything if the open file is valid
		if this.err_code != 1:
			read = open(this.file_name, "r")
			write = open(".simpdat_buf.simpdat", "w")
			found = False

			for line in read:
				if line != "" and line != "\n" and line[0] != '#':
					current_id = ""

					for i in range(len(line)):
						if line[i] == ':':
							break
						current_id += line[i]

					if identifier == current_id:
						try:
							if val_type == "bool" or val_type == "boolean":
								if new_val == True:
									write.write(current_id + ": true\n")
								else:
									write.write(current_id + ": false\n")
							elif val_type == "char":
								write.write(current_id + ": \'" + new_val + "\'\n")
							elif val_type == "string":
								write.write(current_id + ": \"" + new_val + "\"\n")
							else:
								write.write(current_id + ": " + new_val + "\n")
							found = True
						except TypeError:
							if os.name == "posix":
								subprocess.run("rm", ".simpdat_buf.simpdat")
							else:
								subprocess.run("del", "/f", ".simpdat_buf.simpdat")

							return 3
					else:
						write.write(line)
				else:
					write.write(line)

			os.rename(".simpdat_buf.simpdat", this.file_name);

			if found == True:
				this.err_code = 0
			else:
				this.err_code = 2

			return this.err_code


# All functions appended with "simpledata_", and therefore non-dependent to the simpledata class, should be defined here (apart from simpledata_remove_trailing() and simpledata_remove_leading())
def simpledata_version() -> str:
	return SIMPLEDATA_API_VERSION

def simpledata_errfind(errnum) -> str:
	i = 0
	while i < len(errs):
		if errs[i][0] == errnum:
			return errs[i][1]

		i += 1
	return "Not An Error"


# Non class-dependent versions of update() and fetch()
# simpledata_errfind() will have to be used to get these functions' return value description
class simpledata_fetch_ret():
	errcode = 0
	value = None

	def __init__(this, ret_num, val):
		this.errcode = ret_num
		this.value = val

def simpledata_fetch(identifier, filename, type) -> simpledata_fetch_ret:
	try:
		file = open(filename, "r");
		found = False

		# Only outside of the for loop so it can be returned after error code processing
		val_buffer = ""

		for line in file:
			if line != "" and line != "\n" and line[0] != '#':
				current_id = ""
				id_end = 0

				# Getting the identifier
				while id_end < len(line) and line[id_end] != ':':
					current_id += line[id_end];
					id_end += 1

				current_id = simpledata_remove_leading(current_id)
				current_id = simpledata_remove_trailing(current_id);

				if identifier != current_id:
					continue
				else:
					found = True

				# Getting the value
				i = id_end + 1
				while i < len(line):
					val_buffer += line[i]
					i += 1

				val_buffer = simpledata_remove_leading(val_buffer)
				val_buffer = simpledata_remove_trailing(val_buffer)
				break

		file.close()
		if found == True:
			try:
				if type == "int":
					return simpledata_fetch_ret(0, int(val_buffer))
				elif type == "float":
					return simpledata_fetch_ret(0, float(val_buffer))
				elif type == "bool":
					if val_buffer == "true":
						return simpledata_fetch_ret(0, True)
					else:
						return simpledata_fetch_ret(0, False)
				elif type == "char":
					return simpledata_fetch_ret(0, val_buffer[1])
				else:
					i = 1
					str_buffer = ""

					while i < len(val_buffer) - 1:
						str_buffer += val_buffer[i]
						i += 1

					return simpledata_fetch_ret(0, str_buffer)
			except ValueError:
				return simpledata_fetch_ret(3, None)
		else:
			return simpledata_fetch_ret(2, None)

	except FileNotFoundError:
		return simpledata_fetch_ret(1, None)

def simpledata_update(identifier, new_val, val_type, filename) -> int:
	try:
		read = open(filename, "r")
		write = open(".simpdat_buf.simpdat", "w")
		found = False

		for line in read:
			if line != "" and line != "\n" and line[0] != '#':
				current_id = ""

				for i in range(len(line)):
					if line[i] == ':':
						break
					current_id += line[i]

				if identifier == current_id:
					try:
						if val_type == "bool" or val_type == "boolean":
							if new_val == True:
								write.write(current_id + ": true\n")
							else:
								write.write(current_id + ": false\n")
						elif val_type == "char":
							write.write(current_id + ": \'" + new_val + "\'\n")
						elif val_type == "string":
							write.write(current_id + ": \"" + new_val + "\"\n")
						else:
							write.write(current_id + ": " + new_val + "\n")
						found = True
					except TypeError:
						if os.name == "posix":
							subprocess.run(["rm", ".simpdat_buf.simpdat"])
						else:
							subprocess.run("del", "/f", ".simpdat_buf.simpdat")

						return 3
				else:
					write.write(line)
			else:
				write.write(line)

		os.rename(".simpdat_buf.simpdat", filename);

		if found == True:
			return 0
		else:
			return 2
	except FileNotFoundError:
		return 1