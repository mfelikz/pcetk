# Test script for the new module

from pBabel import CHARMMParameterFiles_ToParameters, CHARMMPSFFile_ToSystem, CHARMMCRDFile_ToCoordinates3, PDBFile_FromSystem

from pCore import Pickle, Unpickle, logFile

from ContinuumElectrostatics import MEADModel, StateVector


logFile.Header ("A system with only one titratable site.")


#===========================================
par_tab = ["charmm/par_all27_prot_na.inp", ]

mol  = CHARMMPSFFile_ToSystem ("charmm/testpeptide_xplor.psf", isXPLOR = True, parameters = CHARMMParameterFiles_ToParameters (par_tab))

mol.coordinates3 = CHARMMCRDFile_ToCoordinates3 ("charmm/testpeptide.crd")


#===========================================
ce_model = MEADModel (meadPath = "/home/mikolaj/local/bin/", scratch = "scratch", nthreads = 2)

ce_model.Initialize (mol)

ce_model.Summary ()

ce_model.SummarySites ()

ce_model.WriteJobFiles (mol)

ce_model.CalculateEnergies ()

ce_model.WriteGintr ()

ce_model.WriteW ()

Pickle ("ce_model.pkl", ce_model)

ce_model.CalculateProbabilitiesAnalytically (pH =  4.0)

ce_model.CalculateProbabilitiesAnalytically (pH =  7.0)

ce_model.CalculateProbabilitiesAnalytically (pH = 10.0)


#===========================================
# vector    = StateVector (ce_model)
# 
# vector.Reset ()
# moreIncrements = True
# 
# while moreIncrements:
#   Gmicro  = ce_model.CalculateMicrostateEnergy (vector)
#   message = "Gmicro = %.6f" % Gmicro
# 
#   vector.Print (ce_model, title = message)
#   moreIncrements = vector.Increment ()


#===========================================
logFile.Footer ()
