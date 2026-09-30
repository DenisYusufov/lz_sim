import pkg_resources
import numpy as np
import mercury
from .logger import Logger 

resource_package = __name__

class AlgorithmResults:
	def __init__(self):
		# Initialize all results as None or empty arrays
		self.stat = np.array([])
		self.x = np.array([])
		self.y = np.array([])
		self.chi2 = np.array([])
		self.e = np.array([])
		self.dof = np.array([])
		self.cxx = np.array([])
		self.cyy = np.array([])
		self.cxy = np.array([])

	def updateResults(self, mercury):
		self.stat = np.array(mercury.getRecStatus())
		self.x = np.array(mercury.getRecX())
		self.y = np.array(mercury.getRecY())
        
		self.e = np.array(mercury.getRecE())
		self.dof = np.array(mercury.getDof())
		self.chi2 = np.array(mercury.getChi2Min())/self.dof
		self.cxx = np.array(mercury.getCovXX())
		self.cyy = np.array(mercury.getCovYY())
		self.cxy = np.array(mercury.getCovXY())

class PyMercury(object):


	def __init__(self):
		self._mercury = None
		self._topLRF = None
		self._satFlags = [40000]*253
		self.results = AlgorithmResults()

		resource_path = '/'.join(('resources', "sat_flag.txt"))  # Do not use os.path.join()
		fpath = pkg_resources.resource_filename(resource_package, resource_path)

		self._satParams = np.loadtxt(fpath)

		for line in self._satParams:
			pmti, A, x0, Aerr, x0err = line
			pmti = int(pmti)
			if A <=0:
				continue

			self._satFlags[pmti] = x0

	def getTopLRF(self):
		return self._topLRF

	def setTopLRF(self, lrf):
		self._topLRF = lrf

	def makeMercury(self):
		# Create and set up a parallel ML reconstructor with 12 threads
		self._mercury = mercury.RecML_MP(self.getTopLRF().ModelAsJson(), 12)
		self._mercury.setCogRelCutoff(0.1)
		self._mercury.setRecCutoffRadius(600.)

	def getSatFlags(self):
		return self._satFlags

	def setSatFlags(self, flags):
		self._satFlags = flags

	def reconstruct(self, tops2):
		#print("No sat")
		tops2 = np.array(tops2)
		topsat = (tops2<0) | (tops2>40000)

		for pm, sat_flag in enumerate(self.getSatFlags()):

			if sat_flag<0:
				sat_flag = 40000
			sated = tops2[:,pm]>sat_flag
			topsat[:,pm][sated] = True

		self._mercury.ProcessEvents(tops2, topsat)
		self.results.updateResults(self._mercury)

	def getResults(self):
		return self.results
