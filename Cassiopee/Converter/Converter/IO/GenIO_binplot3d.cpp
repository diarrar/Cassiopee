/*    
    Copyright 2013-2026 ONERA.

    This file is part of Cassiopee.

    Cassiopee is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cassiopee is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cassiopee.  If not, see <http://www.gnu.org/licenses/>.
*/
// nasa plot3d binary file support

# include "Array/Array.h"
# include "GenIO.h"
# include <algorithm>
# include <limits>
# include <stdio.h>
# include <string>
# include <string.h>

using namespace K_ARRAY;
using namespace K_FLD;
using namespace std;

template <typename T>
static bool readValues(FILE* file, T* values, size_t count, bool swapEndian)
{
  if (fread(values, sizeof(T), count, file) != count) return false;
  if (swapEndian)
  {
    char* bytes = reinterpret_cast<char*>(values);
    for (size_t i = 0; i < count; ++i, bytes += sizeof(T))
      std::reverse(bytes, bytes + sizeof(T));
  }
  return true;
}

template <typename T>
static bool readConvertedValues(FILE* file, E_Float* dst, size_t count, bool swapEndian)
{
  const size_t blockSize = 4096;
  T buf[blockSize];
  for (size_t offset = 0; offset < count;)
  {
    size_t block = std::min(blockSize, count - offset);
    if (!readValues(file, buf, block, swapEndian)) return false;
    for (size_t i = 0; i < block; ++i) dst[offset + i] = static_cast<E_Float>(buf[i]);
    offset += block;
  }
  return true;
}

static bool readReals(FILE* file, E_Float* dst, size_t count, E_Int realLength,
                      bool swapEndian)
{
  if (realLength == sizeof(E_Float)) return readValues(file, dst, count, swapEndian);
  if (realLength == sizeof(double)) return readConvertedValues<double>(file, dst, count, swapEndian);
  return readConvertedValues<float>(file, dst, count, swapEndian);
}

static bool readMarker(FILE* file, int expectedSize, bool swapEndian)
{
  int marker;
  return readValues(file, &marker, 1, swapEndian) && marker == expectedSize;
}

static bool readPositiveInteger(FILE* file, E_Int& value, E_Int intLength, bool swapEndian)
{
  if (intLength == sizeof(int))
  {
    int word;
    if (!readValues(file, &word, 1, swapEndian) || word <= 0) return false;
    value = word;
  }
  else
  {
    E_LONG word;
    if (!readValues(file, &word, 1, swapEndian) || word <= 0 ||
        word > std::numeric_limits<E_Int>::max()) return false;
    value = static_cast<E_Int>(word);
  }
  return true;
}

static bool readZoneHeader(FILE* file, E_Int intLength, bool swapEndian,
                           vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
                           vector<E_Int>* nvar = NULL)
{
  const bool haveDimensions = !ni.empty();
  const int bytesPerZone = intLength * (nvar == NULL ? 3 : 4);
  E_Int nzones;
  if (!readMarker(file, intLength, swapEndian) ||
      !readPositiveInteger(file, nzones, intLength, swapEndian) ||
      !readMarker(file, intLength, swapEndian) ||
      nzones > std::numeric_limits<int>::max() / bytesPerZone ||
      (haveDimensions && ni.size() != size_t(nzones))) return false;

  const int headerSize = nzones * bytesPerZone;
  if (!readMarker(file, headerSize, swapEndian)) return false;

  const E_Int maxPoints = std::numeric_limits<int>::max() / sizeof(float);
  for (E_Int i = 0; i < nzones; i++)
  {
    E_Int ip, jp, kp;
    if (!readPositiveInteger(file, ip, intLength, swapEndian) ||
        !readPositiveInteger(file, jp, intLength, swapEndian) ||
        !readPositiveInteger(file, kp, intLength, swapEndian) ||
        ip > maxPoints/jp || ip*jp > maxPoints/kp) return false;
    if (nvar != NULL)
    {
      E_Int nv;
      if (!readPositiveInteger(file, nv, intLength, swapEndian) ||
          (i != 0 && nv != (*nvar)[0])) return false;
      nvar->push_back(nv);
    }
    if (haveDimensions)
    {
      if (ip != ni[i] || jp != nj[i] || kp != nk[i]) return false;
    }
    else
    {
      ni.push_back(ip);
      nj.push_back(jp);
      nk.push_back(kp);
    }
  }
  return readMarker(file, headerSize, swapEndian);
}

//=============================================================================
/* 
   Find xyz, q, and f file name if they exists in file.
   Its the caller responsability to free the returned char*.
   IN: file: file to be opened
   OUT: xyzFile, qFile, fFile: real file name that can be opened
   Retourne le nombre de fichiers que l'on peut ouvrir (0,1,2 ou 3).
   
   Regle:
   Si le fichier file existe, on determine si c'est un xyz, q ou f file
   suivant l'extension:
   - .gbin: xyz
   - .qbin: q
   - .fbin: f
   Sinon, on compose un nom file + .gbin, file + .qbin, file + .fbin, et
   on regarde si ces fichiers existent.
*/
//=============================================================================
E_Int K_IO::GenIO::findOpenablePlot3dFiles(
  char* file,
  char*& xyzFile, char*& qFile, char*& fFile)
{
  FILE* ptrFile;
  xyzFile = NULL; qFile = NULL; fFile = NULL;
  char temp[120];
  E_Bool used;
  
  E_Int openable = 0;
  
  // Analyse la chaine file, decoupe suivant les virgules
  char split[3][120];
  E_Int c = 0;
  E_Int n = 0;
  E_Int l = 0;
  while (file[c] != '\0')
  {
    split[n][l] = file[c];
    if (file[c] == ',') // multiple files in string
    {
      split[n][l] = '\0';
      n++;
      if (n >= 4)
      {
        printf("Warning: too many file names provided. Only 3 are taken into account.\n");
        n--;
        break;
      }
      l = 0;
    }
    else
      l++;
    c++;
  }
  split[n][l] = '\0';
  
  // Examine chaque nom de fichier, existe-t-il?
  for (E_Int i = 0; i <= n; i++)
  {
    used = false;
    l = strlen(split[i]);
    
    ptrFile = fopen(split[i], "rb");

    // Le fichier peut-etre ouvert
    if (ptrFile != NULL)
    {
      used = true;
      if (l >= 4)
      {
        temp[0] = split[i][l-4]; temp[1] = split[i][l-3];  
        temp[2] = split[i][l-2]; temp[3] = split[i][l-1];
        temp[4] = '\0';
        if (strcmp(temp, "gbin") == 0)
        {
          if (xyzFile == NULL)
          {
            xyzFile = new char[120];
            openable++;
          }
          else
            printf("Warning: multiple file names given for xyz definition. Discarding previous one.\n");
          strcpy(xyzFile, split[i]);
        }
        else if (strcmp(temp, "qbin") == 0)
        {
          if (qFile == NULL)
          {
            qFile = new char[120];
            openable++;
          }
          else
            printf("Warning: multiple file names given for q definition. Discarding previous one.\n");
          strcpy(qFile, split[i]);
        }
        else if (strcmp(temp, "fbin") == 0)
        {
          if (fFile == NULL)
          {
            fFile = new char[120];
            openable++;
          }
          else
            printf("Warning: multiple file names given for f definition. Discarding previous one.\n");
          strcpy(fFile, split[i]);
        }
        else
        {
          printf("Warning: findOpenablePlot3dFiles: assuming that file is an xyz plot 3d file.\n");
          xyzFile = new char[120];
          openable++;
          strcpy(xyzFile, split[i]);
        }
      }
      else
      {
        printf("Warning: findOpenablePlot3dFiles: assuming that file is an xyz  plot 3d file.\n");
        xyzFile = new char[120];
        openable++;
        strcpy(xyzFile, split[i]);
      }
      fclose(ptrFile);
    }
    else
    {
      // Le nom n'existe pas. On verifie s'il s'agit d'un nom generique
      strcpy(temp, split[i]);
      strcat(temp, ".gbin");
      ptrFile = fopen(temp, "rb");
      
      if (ptrFile != NULL)
      {
        used = true;
        xyzFile = new char[120];
        openable++;
        strcpy(xyzFile, temp);
        fclose(ptrFile);
      }
      
      strcpy(temp, split[i]);
      strcat(temp, ".qbin");
      ptrFile = fopen(temp, "rb");
      
      if (ptrFile != NULL)
      {
        used = true;
        qFile = new char[120];
        openable++;
        strcpy(qFile, temp);
        fclose(ptrFile);
      }
      
      strcpy(temp, split[i]);
      strcat(temp, ".fbin");
      ptrFile = fopen(temp, "rb");
      
      if (ptrFile != NULL)
      {
        used = true;
        fFile = new char[120];
        openable++;
        strcpy(fFile, temp);
        fclose(ptrFile);
      }
    }
    if (used == false)
      printf("Warning: findOpenablePlot3dFiles: the name %s in file string can not be used.\n", split[i]);
  } // pour chaque nom splitte
  
  return openable;
}

//=============================================================================
// Trouve les noms reels de fichiers a ecrire.
// IN: file: nom du fichier
// IN: varString: chaine des variables
// OUT: xyzFile, qFile, fFile: nom des fichiers reels a ecrire.
//=============================================================================
E_Int K_IO::GenIO::findWritablePlot3dFiles(
  char* file,
  char* varString,
  char*& xyzFile, char*& qFile, char*& fFile)
{
  char temp[120];
  E_Int writable = 0;

  // Analyse la chaine file, decoupe suivant les virgules
  char split[3][120];
  E_Int c = 0;
  E_Int n = 0;
  E_Int l = 0;
  while (file[c] != '\0')
  {
    split[n][l] = file[c];
    if (file[c] == ',') // multiple files in string
    {
      split[n][l] = '\0';
      n++;
      if (n >= 4)
      {
        printf("Warning: you have provided too much file names. Only 3 are taken into account.\n");
        n--;
        break;
      }
      l = 0;
    }
    else
      l++;
    c++;
  }
  split[n][l] = '\0';
  
  E_Int resx = isCoordinateXPresent(varString);
  E_Int resy = isCoordinateYPresent(varString);
  E_Int resz = isCoordinateZPresent(varString);
  //E_Int rescelln = isNamePresent("cellN", varString);
  //if (rescelln == -1) rescelln = isNamePresent("cellnf", varString);
  E_Int resro = isDensityPresent(varString);
  E_Int resrou = isMomentumXPresent(varString);
  E_Int resrov = isMomentumYPresent(varString);
  E_Int resrow = isMomentumZPresent(varString);
  E_Int resroE = isEnergyStagnationDensityPresent(varString);

  // Examine le nom du fichier
  for (E_Int i = 0; i <= n; i++)
  {
    // xyz files
    if (resx != -1 && resy != -1 && resz != -1)
    {
      l = strlen(split[i]);
      if (l >= 4)
      {
        temp[0] = split[i][l-4]; temp[1] = split[i][l-3];  
        temp[2] = split[i][l-2]; temp[3] = split[i][l-1];
        temp[4] = '\0';
        if (strcmp(temp, "gbin") == 0)
        {
          if (xyzFile == NULL)
          {
            xyzFile = new char[120];
            writable++;
          }
          else
            printf("Warning: you gave multiple file names for xyz definition. Discarding previous one.\n");
          strcpy(xyzFile, split[i]);
        }
      }
    }

    // q file
    if (resro != -1 && resrou != -1 && resrov != -1 &&
        resrow != -1 && resroE != -1 )
    {
      l = strlen(split[i]);
      if (l >= 4)
      {
        temp[0] = split[i][l-4]; temp[1] = split[i][l-3];  
        temp[2] = split[i][l-2]; temp[3] = split[i][l-1];
        temp[4] = '\0';
        if (strcmp(temp, "qbin") == 0)
        {
          if (qFile == NULL)
          {
            qFile = new char[120];
            writable++;
          }
          else
            printf("Warning: you gave multiple file names for q definition. Discarding previous one.\n");
          strcpy(qFile, split[i]);
        }
      }
    }
    
    // f file
    l = strlen(split[i]);
    if (l >= 4)
    {
      temp[0] = split[i][l-4]; temp[1] = split[i][l-3];  
      temp[2] = split[i][l-2]; temp[3] = split[i][l-1];
      temp[4] = '\0';
      if (strcmp(temp, "fbin") == 0)
      {
        if (fFile == NULL)
        {
          fFile = new char[120];
          writable++;
        }
        else
          printf("Warning: you gave multiple file names for f definition. Discarding previous one.\n");
        strcpy(fFile, split[i]);
      }
    }
  } // pour chaque nom splitte
  
  // Examine le nom du fichier
  for (E_Int i = 0; i <= n; i++)
  {
    if (xyzFile == NULL && resx != -1 && resy != -1 && resz != -1)
    {
      // nom generique
      xyzFile = new char[120];
      strcpy(xyzFile, split[i]);
      strcat(xyzFile, ".gbin");
      writable++;
    }
    
    if (qFile == NULL && resro != -1 && resrou != -1 && resrov != -1 &&
        resrow != -1 && resroE != -1 )
    {
      qFile = new char[120];
      writable++;
      strcpy(qFile, split[i]);
      strcat(qFile, ".qbin");
    }

    if (fFile == NULL)
    {
      fFile = new char[120];
      writable++;
      strcpy(fFile, split[i]);
      strcat(fFile, ".fbin");
    }
  }
  
  return writable;
}

//=============================================================================
/* 
   Read xyz file.
 */
//=============================================================================
E_Int K_IO::GenIO::xyzread(
  char* file, char*& varString,
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field)
{
  FILE* ptrFile;

  // Check endianess
  E_Int ret = v3dCheckEndian(file); // similar to v3d
  if (ret == -1)
  {
    printf("Warning: xyzread: can not open file %s.\n", file);
    return 1;
  }
  if (ret == -2)
  {
    printf("Warning: xyzread: format in file %s is not recognised.\n", file);
    return 1;
  }
  else if (ret == 0)
  {
    _convertEndian = false;
    _intLength = 4;
  }
  else if (ret == 1)
  {
    _convertEndian = false;
    _intLength = 8;
  }
  else if (ret == 2)
  {
    _convertEndian = true;
    _intLength = 4;
  }
  else
  {
    _convertEndian = true;
    _intLength = 8;
  }

  ptrFile = fopen(file, "rb");
  if (ptrFile == NULL) return 1;
  if (!readZoneHeader(ptrFile, _intLength, _convertEndian, ni, nj, nk))
  {
    printf("Warning: xyzread: invalid or incompatible zone header.\n");
    fclose(ptrFile);
    return 1;
  }
  E_Int nzones = ni.size();
  E_Int iblank = false;

  // Read x,y,z coordinates
  for (E_Int i = 0; i < nzones; i++)
  {
    int recordSize;
    E_Int np = ni[i]*nj[i]*nk[i];
    if (!readValues(ptrFile, &recordSize, 1, _convertEndian) ||
        recordSize <= 0 || recordSize % np != 0)
    {
      printf("Warning: xyzread: invalid coordinate record size.\n");
      fclose(ptrFile);
      return 1;
    }
    E_Int bytesPerPoint = recordSize / np;
    E_Int zoneBlank = false;

    if (bytesPerPoint == 3*8+_intLength)
    {
      _realLength = 8;
      zoneBlank = true;
    }
    else if (bytesPerPoint == 3*4+_intLength)
    {
      _realLength = 4;
      zoneBlank = true;
    }
    else if (bytesPerPoint == 3*8)
    {
      _realLength = 8;
    }
    else if (bytesPerPoint == 3*4)
    {
      _realLength = 4;
    }
    else
    {
      printf("Warning: the file is corrupted.\n");
      fclose(ptrFile);
      return 1;
    }
    if (i > 0 && zoneBlank != iblank)
    {
      printf("Warning: xyzread: inconsistent iblank across zones.\n");
      fclose(ptrFile);
      return 1;
    }
    iblank = zoneBlank;

    FldArrayF* f = new FldArrayF(np, iblank ? 4 : 3);
    field.push_back(f);
    bool ok = readReals(ptrFile, f->begin(), 3*size_t(np), _realLength, _convertEndian);
    if (ok && iblank)
      ok = (_intLength == 8) ?
        readConvertedValues<E_LONG>(ptrFile, f->begin(4), np, _convertEndian) :
        readConvertedValues<int>(ptrFile, f->begin(4), np, _convertEndian);
    if (!ok || !readMarker(ptrFile, recordSize, _convertEndian))
    {
      printf("Warning: xyzread: invalid coordinate record.\n");
      fclose(ptrFile);
      return 1;
    }
  }

  fclose(ptrFile);

  // Build varString
  if (iblank == true)
    strcpy(varString, "x,y,z,cellN");
  else
    strcpy(varString, "x,y,z");
  return 0;
}

//=============================================================================
/* Read q file. */
//=============================================================================
E_Int K_IO::GenIO::p3dqread(
  char* file, char*& varString,
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field)
{
  FILE* ptrFile;

  E_Int xyzdone = false;
  if (ni.size() != 0) xyzdone = true;

  // Check endianess
  E_Int ret = v3dCheckEndian(file); // similar to v3d
  if (ret == -1)
  {
    printf("Warning: qread: can not open file %s.\n", file);
    return 1;
  }
  if (ret == -2)
  {
    printf("Warning: qread: format in file %s is not recognised.\n", file);
    return 1;
  }
  else if (ret == 0)
  {
    _convertEndian = false;
    _intLength = 4;
  }
  else if (ret == 1)
  {
    _convertEndian = false;
    _intLength = 8;
  }
  else if (ret == 2)
  {
    _convertEndian = true;
    _intLength = 4;
  }
  else
  {
    _convertEndian = true;
    _intLength = 8;
  }

  ptrFile = fopen(file, "rb");
  if (ptrFile == NULL) return 1;
  if (!readZoneHeader(ptrFile, _intLength, _convertEndian, ni, nj, nk))
  {
    printf("Warning: qread: invalid or incompatible zone header.\n");
    fclose(ptrFile);
    return 1;
  }
  E_Int nzones = ni.size();

  // Read ro, rou, rov, row, roE
  for (E_Int i = 0; i < nzones; i++)
  {
    int recordSize;
    if (!readValues(ptrFile, &recordSize, 1, _convertEndian))
    {
      fclose(ptrFile);
      return 1;
    }
    E_Int realLength = 0; // precision of the file, from the separator (4 reals)
    if (recordSize == 4*8) realLength = 8;
    else if (recordSize == 4*4) realLength = 4;
    else // sep 4 * taille float
    {
      printf("Warning: binary file corrupted.\n");
      fclose(ptrFile);
      return 1;
    }

    char parameters[4*sizeof(double)]; // Mach, etc.
    if (fread(parameters, 1, recordSize, ptrFile) != size_t(recordSize) ||
        !readMarker(ptrFile, recordSize, _convertEndian))
    {
      printf("Warning: binary file corrupted.\n");
      fclose(ptrFile);
      return 1;
    }

    E_Int ip = ni[i];
    E_Int jp = nj[i];
    E_Int kp = nk[i];
    E_Int np = ip*jp*kp;
    if (!readValues(ptrFile, &recordSize, 1, _convertEndian) ||
        recordSize <= 0 || recordSize % (5*realLength) != 0 ||
        np != recordSize / (5*realLength))
    {
      printf("Warning: binary file corrupted.\n");
      fclose(ptrFile);
      return 1;
    }
    
    E_Int start = 1;
    if (xyzdone == false)
    {
      FldArrayF* f = new FldArrayF(np, 5);
      field.push_back(f);
    }
    else
    {
      FldArrayF* f = field[i];
      E_Int size = f->getSize();
      E_Int nfld = f->getNfld();
      f->reAlloc(size, 5+nfld);
      if (nfld == 4)
      {
        // Deplacement de cellN a la fin
        E_Float* pf = f->begin();
        for (E_Int p = 0; p < size; p++)
          pf[8*size+p] = pf[3*size+p];
      }
      start = (nfld == 4) ? nfld : nfld+1;
    }
    if (!readReals(ptrFile, field[i]->begin(start), recordSize/realLength, realLength, _convertEndian) ||
        !readMarker(ptrFile, recordSize, _convertEndian))
    {
      printf("Warning: qread: invalid field record.\n");
      fclose(ptrFile);
      return 1;
    }
  }
  fclose(ptrFile);

  // Build varString
  if (xyzdone == false)
    strcpy(varString, "ro,rou,rov,row,roE");
  else
  {
    if (field[0]->getNfld() == 9)
    {
      strcpy(varString, "x,y,z,ro,rou,rov,row,roE,cellN");
    }
    else
      strcpy(varString, "x,y,z,ro,rou,rov,row,roE");
  }
  return 0;
}

//=============================================================================
E_Int K_IO::GenIO::p3dfread(
  char* file, char*& varString,
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field)
{
  FILE* ptrFile;
  vector<E_Int> nvar;

  // Un fichier a deja ete lu
  E_Bool xyzdone = false;
  if (ni.size() != 0)
    xyzdone = true;

  // Le cellN a ete lu
  E_Bool cellN = false;
  if (xyzdone == true)
  {
    E_Int l = strlen(varString);
    if (l >= 5 && varString[l-5] == 'c' && varString[l-4] == 'e' &&
        varString[l-3] == 'l' && varString[l-2] == 'l' &&
        varString[l-1] == 'N')
      cellN = true;
  }
  
  // Check endianess
  E_Int ret = v3dCheckEndian(file); // similar to v3d
  if (ret == -1)
  {
    printf("Warning: fread: can not open file %s.\n", file);
    return 1;
  }
  if (ret == -2)
  {
    printf("Warning: fread: format in file %s is not recognised.\n", file);
    return 1;
  }
  else if (ret == 0)
  {
    _convertEndian = false;
    _intLength = 4;
  }
  else if (ret == 1)
  {
    _convertEndian = false;
    _intLength = 8;
  }
  else if (ret == 2)
  {
    _convertEndian = true;
    _intLength = 4;
  }
  else
  {
    _convertEndian = true;
    _intLength = 8;
  }

  ptrFile = fopen(file, "rb");
  if (ptrFile == NULL) return 1;
  if (!readZoneHeader(ptrFile, _intLength, _convertEndian, ni, nj, nk, &nvar))
  {
    printf("Warning: fread: invalid or incompatible zone header.\n");
    fclose(ptrFile);
    return 1;
  }
  E_Int nzones = ni.size();
  std::string names(varString);
  if (cellN) names.resize(names.size()-6);
  for (E_Int p = 0; p < nvar[0]; p++)
  {
    if (!names.empty()) names += ',';
    names += "var" + std::to_string(p);
    if (names.size() + (cellN ? 6 : 0) >= K_ARRAY::VARSTRINGLENGTH)
    {
      printf("Warning: fread: too many variables.\n");
      fclose(ptrFile);
      return 1;
    }
  }
  if (cellN) names += ",cellN";

  // Read additional variables
  for (E_Int i = 0; i < nzones; i++)
  {
    E_Int ip = ni[i];
    E_Int jp = nj[i];
    E_Int kp = nk[i];
    E_Int nv = nvar[i];
    E_Int np = ip*jp*kp;
    int recordSize;
    if (!readValues(ptrFile, &recordSize, 1, _convertEndian) || recordSize <= 0 ||
        recordSize % np != 0 || (recordSize / np) % nv != 0)
    {
      printf("Warning: fread: invalid field record size.\n");
      fclose(ptrFile);
      return 1;
    }
    E_Int realLength = recordSize / np / nv;
    if (realLength != sizeof(float) && realLength != sizeof(double))
    {
      printf("Warning: fread: unsupported real width.\n");
      fclose(ptrFile);
      return 1;
    }
    E_Int start = 1;
    if (xyzdone == false)
    {
      FldArrayF* f = new FldArrayF(np, nv);
      field.push_back(f);
    }
    else
    {
      FldArrayF* f = field[i];
      E_Int size = f->getSize();
      E_Int nfld = f->getNfld();
      f->reAlloc(size, nv+nfld);
      if (cellN == true)
      {
        // Deplacement de cellN a la fin
        E_Float* pf = f->begin();
        for (E_Int p = 0; p < size; p++)
          pf[(nfld+nv-1)*size+p] = pf[(nfld-1)*size+p];
      }
      start = cellN ? nfld : nfld+1;
    }
    if (!readReals(ptrFile, field[i]->begin(start), recordSize/realLength, realLength, _convertEndian) ||
        !readMarker(ptrFile, recordSize, _convertEndian))
    {
      printf("Warning: fread: invalid field record.\n");
      fclose(ptrFile);
      return 1;
    }
  }
  fclose(ptrFile);
  strcpy(varString, names.c_str());
  return 0;
}

//=============================================================================
/* plot3dread.
   Read binary plot3d file as generated by ifort compiler. */
//=============================================================================
E_Int K_IO::GenIO::plot3dread(
  char* file, char*& varString,
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field, vector<char*>& zoneNames)
{
  varString = new char [K_ARRAY::VARSTRINGLENGTH];
  varString[0] = '\0';

  // Find the openable files
  char* xyzFile = NULL;
  char* qFile = NULL;
  char* fFile = NULL;
  E_Int res = findOpenablePlot3dFiles(file, xyzFile, qFile, fFile);
  
  if (res == 0) res = 1; // no openable file
  else
  {
    res = 0;
    if (xyzFile != NULL) res = xyzread(xyzFile, varString, ni, nj, nk, field);
    if (res == 0 && qFile != NULL) res = p3dqread(qFile, varString, ni, nj, nk, field);
    if (res == 0 && fFile != NULL) res = p3dfread(fFile, varString, ni, nj, nk, field);
  }
  delete [] xyzFile;
  delete [] qFile;
  delete [] fFile;

  if (res != 0)
  {
    for (FldArrayF* f : field) delete f;
    field.clear();
    ni.clear(); nj.clear(); nk.clear();
    delete [] varString;
    varString = NULL;
    return res;
  }

  // Cree les noms de zones
  for (E_Int i = 0; i < E_Int(field.size()); i++)
  {
    char* zoneName = new char [128];
    sprintf(zoneName, "Zone" SF_D_, i);
    zoneNames.push_back(zoneName);
  }

  return 0;
}

//=============================================================================
// xyzwrite
//=============================================================================
E_Int K_IO::GenIO::xyzwrite(
  char* file, char* varString, 
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field, E_Int isize, E_Int rsize, 
  E_Bool convertEndian)
{
  FILE* ptrFile;
  E_Int res, resCellN;

  /* Constants */
  E_Int si = sizeof(int);
  E_Int si2 = sizeof(E_LONG);

  // Open file
  ptrFile = fopen(file, "wb");
  if (ptrFile == NULL)
  {
    printf("Warning: xyzwrite: can not open file %s\n.", file);
    return 1;
  }

  // Get number of variables
  if (field.size() == 0) return 0; // nothing to write
  E_Int nvar = field[0]->getNfld();

  // Sep: taille int
  writeInt(isize, si,
           ptrFile, convertEndian, si, si2);

  // Nombre de zones
  E_Int nzones = field.size();
  writeInt(nzones, isize,
           ptrFile, convertEndian, si, si2);

  // Sep: taille int
  writeInt(isize, si,
           ptrFile, convertEndian, si, si2);

  // Sep: nombre de zones * 3 * taille int
  writeInt(nzones * isize * 3, si,
           ptrFile, convertEndian, si, si2);

  for (E_Int i = 0; i < nzones; i++)
  { 
    writeInt(ni[i], isize,
             ptrFile, convertEndian, si, si2);
    writeInt(nj[i], isize,
             ptrFile, convertEndian, si, si2);
    writeInt(nk[i], isize,
             ptrFile, convertEndian, si, si2);
  }
  
  // Sep : nombre de zones * 3 * taille int
  writeInt(nzones * isize * 3, si,
           ptrFile, convertEndian, si, si2);

  resCellN = isNamePresent("cellN", varString);
  if (resCellN == -1) resCellN = isNamePresent("cellnf", varString);
  if (resCellN == -1) resCellN = isNamePresent("cellNF", varString);

  nvar = 3 * sizeof(E_Float);
  if (resCellN != -1)
    nvar = nvar + isize;

  for (E_Int i = 0; i < nzones; i++)
  {
    // Get buffer
    FldArrayF buf(*field[i]);
    if (convertEndian == true)
      convertEndianField(buf);

    // Sep: taille dom * nvar * taille float
    writeInt(ni[i]*nj[i]*nk[i] * nvar, si,
             ptrFile, convertEndian, si, si2);

    res = isCoordinateXPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);
    res = isCoordinateYPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);
    res = isCoordinateZPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);
    
    if (resCellN != -1)
    {
      if (isize == si)
      {
        int* iblank = new int[ni[i]*nj[i]*nk[i]];
        E_Float* pf = buf.begin(resCellN+1);
        for (E_Int ind = 0; ind < ni[i]*nj[i]*nk[i]; ind++)
          iblank[ind] = (int)pf[ind];
        fwrite(iblank, isize, ni[i]*nj[i]*nk[i], ptrFile);
        delete [] iblank;
      }
      else
      {
        E_LONG* iblank = new E_LONG[ni[i]*nj[i]*nk[i]];
        E_Float* pf = buf.begin(resCellN+1);
        for (E_Int ind = 0; ind < ni[i]*nj[i]*nk[i]; ind++)
          iblank[ind] = (E_LONG)pf[ind];
        fwrite(iblank, isize, ni[i]*nj[i]*nk[i], ptrFile);
        delete [] iblank;
      }
    }

    // Sep: taille dom * nvar * taille float
    writeInt(ni[i]*nj[i]*nk[i] * nvar, si,
             ptrFile, convertEndian, si, si2);
  }

  fclose(ptrFile);
  return 0;  
}

//=============================================================================
// qwrite
//=============================================================================
E_Int K_IO::GenIO::qwrite(
  char* file, char* varString, 
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field, E_Int isize, E_Int rsize, 
  E_Bool convertEndian)
{
  FILE* ptrFile;
  E_Int res;

  /* Constants */
  E_Int si = sizeof(int);
  E_Int si2 = sizeof(E_LONG);

  // Open file
  ptrFile = fopen(file, "wb");
  if (ptrFile == NULL)
  {
    printf("Warning: qwrite: can not open file %s\n.", file);
    return 1;
  }

  // Get number of variables
  if (field.size() == 0) return 0; // nothing to write
  E_Int nvar = field[0]->getNfld();

  // Sep: taille int
  writeInt(isize, si, ptrFile, convertEndian, si, si2);

  // Nombre de zones
  E_Int nzones = field.size();
  writeInt(nzones, isize,
           ptrFile, convertEndian, si, si2);

  // Sep: taille int
  writeInt(isize, si,
           ptrFile, convertEndian, si, si2);

  // Sep: nombre de zones * 3 * taille int
  writeInt(nzones * isize * 3, si,
           ptrFile, convertEndian, si, si2);

  for (E_Int i = 0; i < nzones; i++)
  { 
    writeInt(ni[i], isize,
             ptrFile, convertEndian, si, si2);
    writeInt(nj[i], isize,
             ptrFile, convertEndian, si, si2);
    writeInt(nk[i], isize,
             ptrFile, convertEndian, si, si2);
  }
  
  // Sep: nombre de zones * 3 * taille int
  writeInt(nzones*isize*3, si,
           ptrFile, convertEndian, si, si2);

  nvar = 5;

  for (E_Int i = 0; i < nzones; i++)
  {
    // Get buffer
    FldArrayF buf(*field[i]);
    if (convertEndian == true) convertEndianField(buf);
    
    // Sep : 4 * taille double
    writeInt(4 * sizeof(E_Float), si,
             ptrFile, convertEndian, si, si2);

    // Mach,...
    E_Float value = 0.;
    fwrite(&value, sizeof(E_Float), 1, ptrFile);
    fwrite(&value, sizeof(E_Float), 1, ptrFile);
    fwrite(&value, sizeof(E_Float), 1, ptrFile);
    fwrite(&value, sizeof(E_Float), 1, ptrFile);

    // Sep : 4 * taille double
    writeInt(4 * sizeof(E_Float), si,
             ptrFile, convertEndian, si, si2);

    // Sep : taille dom * 5 * taille float
    writeInt(ni[i]*nj[i]*nk[i] * sizeof(E_Float) * nvar, si,
             ptrFile, convertEndian, si, si2);

    res = isDensityPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);
    res = isMomentumXPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);
    res = isMomentumYPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);
    res = isMomentumZPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile); 
    res = isEnergyStagnationDensityPresent(varString);
    fwrite(buf.begin(res+1), sizeof(E_Float), 
           ni[i]*nj[i]*nk[i], ptrFile);

    // Sep : taille dom * nvar * taille float
    writeInt(ni[i]*nj[i]*nk[i] * sizeof(E_Float) * nvar, si,
             ptrFile, convertEndian, si, si2);
  }

  fclose(ptrFile);
  return 0;
}

//=============================================================================
// fwrite
// Ecrit les autres champs (autre que x,y,z)
// Si writeConservative=True, ecrit aussi les variables conservatives
//=============================================================================
E_Int K_IO::GenIO::ffwrite(
  char* file, char* varString, 
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field, E_Int isize, E_Int rsize, 
  E_Bool convertEndian, E_Bool writeConservative)
{
  FILE* ptrFile;

  /* Constants */
  E_Int si = sizeof(int);
  E_Int si2 = sizeof(E_LONG);

  // Get number of variables
  if (field.size() == 0) return 0; // nothing to write
  E_Int nvar = field[0]->getNfld();

  // Open file
  ptrFile = fopen(file, "wb");
  if (ptrFile == NULL)
  {
    printf("Warning: ffwrite: can not open file %s\n.", file);
    return 1;
  }

  // Sep : taille int
  writeInt(isize, si,
           ptrFile, convertEndian, si, si2);

  // Nombre de zones
  E_Int nzones = field.size();
  writeInt(nzones, isize,
           ptrFile, convertEndian, si, si2);

  // Sep : taille int
  writeInt(isize, si,
           ptrFile, convertEndian, si, si2);

  // Sep : nombre de zones * 3 * taille int
  writeInt(nzones * isize * 3, si,
           ptrFile, convertEndian, si, si2);

  for (E_Int i = 0; i < nzones; i++)
  { 
    writeInt(ni[i], isize,
             ptrFile, convertEndian, si, si2);
    writeInt(nj[i], isize,
             ptrFile, convertEndian, si, si2);
    writeInt(nk[i], isize,
             ptrFile, convertEndian, si, si2);
  }
  
  // Sep : nombre de zones * 3 * taille int
  writeInt(nzones * isize * 3, si,
           ptrFile, convertEndian, si, si2);

  for (E_Int i = 0; i < nzones; i++)
  {
    nvar = field[i]->getNfld();

    // Get buffer
    FldArrayF buf(*field[i]);
    if (convertEndian == true) convertEndianField(buf);
    
    // Sep : 4 * taille double
    writeInt(4 * sizeof(E_Float), si,
             ptrFile, convertEndian, si, si2);

    // Mach,...
    E_Float value = 0.;
    fwrite(&value, sizeof(E_Float), 1, ptrFile);
    fwrite(&value, sizeof(E_Float), 1, ptrFile);
    fwrite(&value, sizeof(E_Float), 1, ptrFile);
    fwrite(&value, sizeof(E_Float), 1, ptrFile);

    // Sep : 4 * taille double
    writeInt(4 * sizeof(E_Float), si,
             ptrFile, convertEndian, si, si2);

    // Sep : taille dom * 5 * taille float
    writeInt(ni[i]*nj[i]*nk[i] * sizeof(E_Float) * nvar, si,
             ptrFile, convertEndian, si, si2);

    // Position des variables 
    E_Int resx = isCoordinateXPresent(varString);
    E_Int resy = isCoordinateYPresent(varString);
    E_Int resz = isCoordinateZPresent(varString);
    E_Int res1 = isDensityPresent(varString);
    E_Int res2 = isMomentumXPresent(varString);
    E_Int res3 = isMomentumYPresent(varString);
    E_Int res4 = isMomentumZPresent(varString);
    E_Int res5 = isEnergyStagnationDensityPresent(varString);

    for (E_Int nv = 1; nv <= nvar; nv++)
    {
      if (writeConservative == false)
      {
        if (nv != resx && nv != resy && nv != resz &&
            nv != res1 && nv != res2 && nv != res3 &&
            nv != res4 && nv != res5)
          fwrite(buf.begin(nv), sizeof(E_Float), 
                 ni[i]*nj[i]*nk[i], ptrFile);
      }
      else
      {
        if (nv != resx && nv != resy && nv != resz)
          fwrite(buf.begin(nv), sizeof(E_Float), 
                 ni[i]*nj[i]*nk[i], ptrFile);
      }
    }

    // Sep : taille dom * nvar * taille float
    writeInt(ni[i]*nj[i]*nk[i] * sizeof(E_Float) * nvar, si,
             ptrFile, convertEndian, si, si2);
  }

  fclose(ptrFile);
  return 0;  
}

//=============================================================================
// plot3dwrite
// write plot3d file, with options
// Return 1 upon failure.
//=============================================================================
E_Int K_IO::GenIO::plot3dwrite(
  char* file, char* dataFmt, char* varString, 
  vector<E_Int>& ni, vector<E_Int>& nj, vector<E_Int>& nk,
  vector<FldArrayF*>& field,
  vector<char*>& zoneNames, E_Int isize, E_Int rsize, 
  E_Bool convertEndian)
{
  // Find writable files
  char* xyzFile = NULL;
  char* qFile = NULL;
  char* fFile = NULL;
  findWritablePlot3dFiles(file, varString,
                          xyzFile, qFile, fFile);

  if (xyzFile != NULL)
  {
    E_Int resx = isCoordinateXPresent(varString);
    E_Int resy = isCoordinateYPresent(varString);
    E_Int resz = isCoordinateZPresent(varString);
    if (resx == -1 || resy == -1 || resz == -1)
    {
      if (resx == -1)
        printf("Warning: xyz can not be written because no x variable was found in array.\n");
      if (resy == -1)
        printf("Warning: xyz can not be written because no y variable was found in array.\n");
      if (resz == -1)
        printf("Warning: xyz can not be written because no z variable was found in array.\n");
    }
    else
    {
      xyzwrite(xyzFile, varString,
               ni, nj, nk, field, isize, rsize, convertEndian);
    }
    delete [] xyzFile;
  }

  if (qFile != NULL)
  {
    E_Int resro = isDensityPresent(varString);
    E_Int resrou = isMomentumXPresent(varString);
    E_Int resrov = isMomentumYPresent(varString);
    E_Int resrow = isMomentumZPresent(varString);
    E_Int resroE = isEnergyStagnationDensityPresent(varString);
    if (resro == -1 || resrou == -1 || resrov == -1 
        || resrow == -1 || resroE == -1)
    {
      if (resro == -1)
        printf("Warning: q file can not be written because no ro variable was found in array.\n");
      if (resrou == -1)
        printf("Warning: q file can not be written because no rou variable was found in array.\n");
      if (resrov == -1)
        printf("Warning: q file can not be written because no rov variable was found in array.\n");
      if (resrow == -1)
        printf("Warning: q file can not be written because no row variable was found in array.\n");
      if (resroE == -1)
        printf("Warning: q file can not be written because no roE variable was found in array.\n");
    }
    else
    {
      qwrite(qFile, varString,
             ni, nj, nk, field, isize, rsize, convertEndian); 
    }
    delete [] qFile;
  }
 
  if (fFile != NULL)
  {
    E_Bool writeConservative = true;
    if (qFile == NULL) writeConservative = false;
    ffwrite(fFile, varString,
            ni, nj, nk, field, isize, rsize, convertEndian, 
            writeConservative); 
    delete [] fFile;
  }

  return 0;

}
