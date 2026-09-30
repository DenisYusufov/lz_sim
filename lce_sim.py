import sys
import os
from typing import List, Tuple, Dict, Union, Callable, Optional, Any, Iterable
import numpy as np
import logging
from scipy.stats import poisson
import pkg_resources
from .logger import Logger 
from . import pywrapper_lrmodel 
from . import BinomialInteractionSimulation as BIS

resource_package = __name__

# Parameters for SR1 bottom PMTs
SR1params_bot = {
    'W': 13.5, 'sigmag': 0.05, 'F': 1,
    'g1': 0.1072, 'g2': 14.51, 'G': 17.07,
    'sigmaspe': 0.51, 'sigmase': 5.32
}

# Parameters for SR1 top PMTs
SR1params_top = {
    'W': 13.5, 'sigmag': 0.05, 'F': 1, 
    'g1': 0.1072, 'g2': 44.21, 'G': 51.76, 
    'sigmaspe': 0.51, 'sigmase': 5.32
}

# Parameters for SR3 bottom PMTs
SR3params_bot = {
    'W': 13.5, 'sigmag': 0.05, 'F': 1,
    'g1': 0.1191, 'g2': 11.80, 'G': 16.07,
    'sigmaspe': 0.51, 'sigmase': 5.32
}

# Parameters for SR3 top PMTs
SR3params_top = {
    'W': 13.5, 'sigmag': 0.05, 'F': 1,
    'g1': 0.1191, 'g2': 34.94, 'G': 47.58,
    'sigmaspe': 0.51, 'sigmase': 5.32
}


def t_lin_b(x: np.ndarray, p0: float, p1: float) -> np.ndarray:
    """
    Linear function for transformation.
    
    Args:
        x: Input data points
        p0: Slope
        p1: Intercept
        
    Returns:
        Transformed values following linear function
    """
    return x * p0 + p1


def hyper_vova_2(x: np.ndarray, A: float, x0: float) -> np.ndarray:
    """
    Hyperbolic tangent function for non-linear transformation of PMT response.
    
    Args:
        x: Input data points
        A: Amplitude parameter
        x0: Offset parameter
        
    Returns:
        Transformed values following hyperbolic tangent function
    """
    B = A - x0
    return B * np.tanh((x - x0) / (A - x0)) + x0


def parted_function_vova(x: np.ndarray, A: float, x0: float) -> np.ndarray:
    """
    Piecewise function that combines linear and hyperbolic tangent functions.
    Used for modeling PMT response curves.
    
    Args:
        x: Input data points
        A: Amplitude parameter for hyperbolic function
        x0: Threshold for switching between linear and hyperbolic functions
        
    Returns:
        Transformed values following the piecewise function
    """
    x = np.atleast_1d(x)
    sel = x < x0
    out = np.zeros(x.shape[0])

    out[sel] = t_lin_b(x[sel], 1, 0)  # Linear below threshold
    out[~sel] = hyper_vova_2(x[~sel], A, x0)  # Hyperbolic above threshold

    return out


class LCESim(object):
    """
    Light Collection Efficiency Simulator for PMT responses in detector environment.
    
    This class simulates the response of Photomultiplier Tubes (PMTs) to 
    energy depositions in a detector, accounting for detector geometry, 
    light collection efficiency, and PMT saturation effects.
    """

    def __init__(self) -> None:
        """
        Initialize the LCESim object with default paths and model parameters.
        Loads LRF (Light Response Function) models and PMT saturation curves.
        """

        self.logger = Logger(name='LCESim', level=logging.INFO).get_logger()

        self.topSim = None
        self.botSim = None

        # Initialize SR1 model
        self.SR1model = pywrapper_lrmodel.PyLRM()

        resource_path = '/'.join(('resources', "TopLRF_SR1.json"))  # Do not use os.path.join()
        fpath = pkg_resources.resource_filename(resource_package, resource_path)
        self.SR1model.setTopLRFPath(fpath)
        self.SR1model.makeTopLRF()

        resource_path = '/'.join(('resources', "BottomLRF_SR1.json"))  # Do not use os.path.join()
        fpath = pkg_resources.resource_filename(resource_package, resource_path)
        self.SR1model.setBottomLRFPath(fpath)
        self.SR1model.makeBottomLRF()

        # Initialize SR3 model
        self.SR3model = pywrapper_lrmodel.PyLRM()

        resource_path = '/'.join(('resources', "TopLRF_SR3.json"))  # Do not use os.path.join()
        fpath = pkg_resources.resource_filename(resource_package, resource_path)
        self.SR3model.setTopLRFPath(fpath)
        self.SR3model.makeTopLRF()

        resource_path = '/'.join(('resources', "BottomLRF_SR3.json"))  # Do not use os.path.join()
        fpath = pkg_resources.resource_filename(resource_package, resource_path)
        self.SR3model.setBottomLRFPath(fpath)
        self.SR3model.makeBottomLRF()

        # Get PMT positions
        self.Xtop = self.SR3model.getTopLRF().GetAllX()
        self.Ytop = self.SR3model.getTopLRF().GetAllY()

        self.Xbot = self.SR3model.getBottomLRF().GetAllX()
        self.Ybot = self.SR3model.getBottomLRF().GetAllY()

        # Load saturation flags and create saturation curves

        resource_path = '/'.join(('resources', "sat_flag.txt"))  # Do not use os.path.join()
        fpath = pkg_resources.resource_filename(resource_package, resource_path)

        self._satFlags = np.loadtxt(fpath)
        self.satcurves = self._makeSaturationCurves(self._satFlags)

    def applyPMTResponseCurve(self, observed: np.ndarray) -> np.ndarray:
        """
        Applies the PMT response curve to account for saturation effects.
        
        Args:
            observed: Simulated PMT response data before saturation correction.
                     Can be either:
                     - list of 253 observed signals where each index is the pmt observed signal
                     - matrix with 253 columns and N rows. Each column is the pmt observed signal 
                       and N'th row represents a given event
        
        Returns:
            Expected (corrected) PMT responses after applying saturation curves
        """
        observed = np.atleast_2d(observed)
        expected = []

        for pmt in range(253):
            expected.append(self.satcurves[pmt](observed[:, pmt]))
        return np.array(expected).T
    
    def _makeSaturationCurves(self, matrix: np.ndarray) -> List[Callable]:
        """
        Create saturation curve functions for each PMT.
        
        Args:
            matrix: Array containing saturation parameters for PMTs
                   Each row contains [pmt_id, A, x0, Aerr, x0err]
        
        Returns:
            List of saturation curve functions for each PMT
        """
        def create_function(A: float, x0: float) -> Callable:
            """Create a closure for the saturation function with fixed parameters."""
            return lambda x: parted_function_vova(x, A, x0)
        
        # Initialize with default saturation function for all PMTs
        funcs = [create_function(120000, 120000)] * 253

        # Apply specific saturation functions for PMTs defined in the matrix
        for line in matrix:
            pmti, A, x0, Aerr, x0err = line
            pmti = int(pmti)
            if A <= 0:
                continue
            funcs[pmti] = create_function(A, x0)
        return funcs

    def getModel(self) -> Any:
        """
        Get the current LRF model.
        
        Returns:
            Current LRF model (either SR1 or SR3)
        """
        return self.model
    
    def useModel(self, model: str) -> None:
        """
        Set which detector model to use (SR1 or SR3).
        
        Args:
            model: Model name ("sr1" or "sr3")
        """
        if model.lower() == "sr1":
            self.model = self.SR1model
            self.topSim = BIS.InteractionSimulator(SR1params_top)
            self.botSim = BIS.InteractionSimulator(SR1params_bot)

        elif model.lower() == "sr3":
            self.model = self.SR3model
            self.topSim = BIS.InteractionSimulator(SR3params_top)
            self.botSim = BIS.InteractionSimulator(SR3params_bot)

        self.logger.info(f"Using {model} model.")

    def TopArrayS2(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        """
        Get normalized top PMT array response for S2 signal at given positions.
        
        Args:
            x: X-coordinates of events (mm)
            y: Y-coordinates of events (mm)
            
        Returns:
            Normalized probability distribution across top PMT array
        """
        # Vectorized evaluation using numpy
        x, y = np.atleast_1d(x), np.atleast_1d(y)  # Ensure x and y are arrays
        nevts = x.shape[0]

        # Evaluate top array response at each (x,y) position
        array = np.array([self.getModel().evalTopArray(xi, yi) for xi, yi in zip(x, y)])
        
        # Normalize by corrected area to account for disabled PMTs
        corrected_area = np.array(self.getModel().getTopLRF().GetCorrectedArea(
            array, np.zeros((nevts, 253)), x, y, [0] * nevts))
        
        # Return normalized probabilities
        return (array.T / corrected_area).T

    def BottomArrayS2(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        """
        Get normalized bottom PMT array response for S2 signal at given positions.
        
        Args:
            x: X-coordinates of events (mm)
            y: Y-coordinates of events (mm)
            
        Returns:
            Normalized probability distribution across bottom PMT array
        """
        x, y = np.atleast_1d(x), np.atleast_1d(y)  # Ensure x and y are arrays
        nevts = x.shape[0]
        
        # Evaluate bottom array response at each (x,y) position
        array = np.array([self.model.evalBottomArray(xi, yi) for xi, yi in zip(x, y)])
        
        # Normalize by corrected area to account for disabled PMTs
        corrected_area = self.getModel().getBottomLRF().GetCorrectedArea(
            array, np.zeros((nevts, 241)), x, y, [0] * nevts)
        
        # Return normalized probabilities
        return (array.T / corrected_area).T
    
    def simScatter(self, x: Union[float, np.ndarray], y: Union[float, np.ndarray], 
                   energy: float, ratio: Optional[np.ndarray] = None) -> Tuple[np.ndarray, np.ndarray]:
        """
        Simulates the response of the PMTs to energy deposition(s) at coordinates (x, y).
        
        Args:
            x: X-coordinate(s) of the event(s) (mm)
            y: Y-coordinate(s) of the event(s) (mm)
            energy: Energy of the event (keV)
            ratio: Optional energy ratio distribution for multiple scatter events
                   If None, energy is distributed equally among all scatter points
        
        Returns:
            s2top: Simulated response of the top PMTs with Poisson noise
            s2bot: Simulated response of the bottom PMTs with Poisson noise
            
        Notes:
            - Poisson noise is included in the simulation
            - Energy is converted from keV to eV for internal calculations
        """
        energy *= 1000  # Convert keV to eV
        x, y = np.atleast_1d(x), np.atleast_1d(y)  # Ensure x and y are arrays
        nevts = x.shape[0]
        
        # If no ratio provided, distribute energy equally
        if ratio is None:
            p = x.shape[0]
            rt = 1.0 / p
            ratio = np.array([rt] * p)

        # Get S2 top and bottom signals using detector parameters
        S1, S2t, E_rec = self.topSim.sim_interaction(energy, 1)
        S1, S2b, E_rec = self.botSim.sim_interaction(energy, 1)

        # Distribute energy among scatters according to ratio
        S2t = (ratio * S2t)
        S2b = (ratio * S2b)

        # Get normalized LRF (Light Response Function) distributions
        s2top_array = np.array([d for d in self.TopArrayS2(x, y)])
        s2bot_array = np.array([d for d in self.BottomArrayS2(x, y)])

        # Apply true LRF to calculated S2 signals
        # This returns a matrix of n PMT x n scatter
        s2top_array = (s2top_array.T * S2t).T
        s2bot_array = (s2bot_array.T * S2b).T

        # Sum all scatter contributions and add Poisson noise to PMT response
        return (poisson.rvs(np.sum(s2top_array, axis=0)), 
                poisson.rvs(np.sum(s2bot_array, axis=0)))