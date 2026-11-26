# - numpy example -
import Template as T
import numpy
import Converter.Internal as Internal

a = numpy.zeros( (120,3), dtype=Internal.E_NpyFloat)
T.template.numpyExample(a)

T.template.numpyExample2(a)
