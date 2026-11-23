import Converter.PyTree as C
import Converter.Internal as Internal
import Converter.Mpi as Cmpi
import Geom.IBM as D_IBM
import Connector.AMR as X_AMR
import KCore.test as test
LOCAL = test.getLocal()
LOCAL +='/'
## ===================
##       INPUTS
## ===================
snear      = 0.002
dFar       = 10
vmins      = [25,24,24,20,21,5,4]
dim        = 2

################### IBM pre-processing #########################
IBM_parameters = {
    "donor points": {
        "front type": "1",
        "depth DonorPoints": 1,  # for type 1
    },
    "integration points": {
        "front type": "1",
        "depth IntegrationPoints": 1,  # for type 1
    },
    "IBM type": {
        "type": "global",
        "size elements body": snear,
    },
    "spatial discretization": {
        "type": "FV",
    },
}

tb = D_IBM.naca0012(snear, 'Musker', 0.)
D_IBM._setDfar(tb, dFar)
Cmpi.barrier()

X_AMR.prepareAMRIBM(tb, vmins, dim, IBM_parameters, OutputAMRMesh=True, check=False, localDir=LOCAL, fileName='tIBM.cgns')

tAMR = C.convertFile2PyTree(LOCAL+'tAMRMesh.cgns')
if Cmpi.rank==0:test.testT(tAMR,1)
del tAMR

tIBM = C.convertFile2PyTree(LOCAL+'tIBM.cgns')
if Cmpi.rank == 0:
    walls = [bc for bc in Internal.getNodesFromType(tIBM, 'BC_t') if bc[0].startswith('IBMWall')]
    assert walls
    for bc in walls:
        first, last = Internal.getNodeFromName1(bc, 'ElementRange')[1].ravel()
        faceCount = last - first + 1
        for name in ('DonorPointCoordinates', 'WallPointCoordinates'):
            coordinates = Internal.getNodeFromName(bc, name)
            for axis in 'XYZ':
                pointCount = Internal.getNodeFromName1(coordinates, 'Coordinate' + axis)[1].size
                assert pointCount == faceCount, f'{bc[0]} {name}/{axis} has {pointCount} points for {faceCount} faces'
    test.testT(tIBM,2)
