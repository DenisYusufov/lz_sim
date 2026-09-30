from .logger import Logger 
import logging
import lrmodel



class PyLRM(object):

	def __init__(self):
		self.logger = Logger(name='PyLRM', level=logging.INFO).get_logger()
		
		self._topLRF = None
		self._bottomLRF = None

		self._topLRFPath = None
		self._bottomLRFPath = None

		self._topDisabledList = []
		self._bottomDisabledList = []

	def getTopLRFPath(self):
		return self._topLRFPath

	def setTopLRFPath(self, value):
		self._topLRFPath = value

	def getBottomLRFPath(self):
		return self._bottomLRFPath

	def setBottomLRFPath(self, value):
		self._bottomLRFPath = value

	def getTopLRF(self):
		return self._topLRF

	def getBottomLRF(self):
		return self._bottomLRF

	def makeTopLRF(self):
		file_path = self.getTopLRFPath()
		try:
			with open(file_path, 'r') as file:
				# Read the entire contents of the file into a string
				json_lrm = file.read()
				self._topLRF = lrmodel.LRModel(json_lrm)
				self._topLRF.SetDisabledList(self._topLRF.GetDisabledList()+self.getTopDisabledList())
				self._topLRF.SetRefPoint(0,0)

				self.logger.debug(f"Successfully imported Top LRF from {file_path}.")
				self.logger.debug(f"List of disabled PMT:{self.getTopLRF().GetDisabledList()}.")

		except FileNotFoundError:
			self.logger.error(f"File '{file_path}' not found.")
		except Exception as e:
			self.logger.error(f"An error occurred while reading file {file_path}: {str(e)}")


	def makeBottomLRF(self):
		file_path = self.getBottomLRFPath()
		try:
			with open(file_path, 'r') as file:
				# Read the entire contents of the file into a string
				json_lrm = file.read()
				self._bottomLRF = lrmodel.LRModel(json_lrm)
				self._bottomLRF.SetDisabledList(self._bottomLRF.GetDisabledList()+self.getBottomDisabledList())
				self._bottomLRF.SetRefPoint(0,0)

				self.logger.debug(f"Successfully imported Bottom LRF from {file_path}.")
				self.logger.debug(f"List of disabled PMT:{self.getBottomLRF().GetDisabledList()}.")

		except FileNotFoundError:
			self.logger.error(f"File '{file_path}' not found.")
		except Exception as e:
			self.logger.error(f"An error occurred: {str(e)}")

	def getTopDisabledList(self):
		return self._topDisabledList

	def setTopDisabledList(self, value):
		self._topDisabledList = value

	def getBottomDisabledList(self):
		return self._bottomDisabledList

	def setBottomDisabledList(self, value):
		self._bottomDisabledList = value

	def evalTopArray(self, x, y):
		return self._topLRF.EvalAll(x, y, 0)
	
	def evalBottomArray(self, x, y):
		return self._bottomLRF.EvalAll(x, y, 0)