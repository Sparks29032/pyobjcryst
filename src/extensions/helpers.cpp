/*****************************************************************************
*
* pyobjcryst        by DANSE Diffraction group
*                   Simon J. L. Billinge
*                   (c) 2009 The Trustees of Columbia University
*                   in the City of New York.  All rights reserved.
*
* File coded by:    Chris Farrow
*
* See AUTHORS.txt for a list of people who contributed.
* See LICENSE_DANSE.txt for license information.
*
******************************************************************************
*
* Implementation of non-template binding-specific helper functions.
*
*****************************************************************************/

#include "helpers.hpp"
#include <boost/python/stl_iterator.hpp>

#include <iostream>
#include <list>

#include <ObjCryst/CrystVector/CrystVector.h>

// Use numpy here, but initialize it later in the extension module.
#include "pyobjcryst_numpy_setup.hpp"
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>


namespace bp = boost::python;

namespace {

void black_hole_info(const std::string& s)  { }

}   // namespace

// class MuteObjCrystUserInfo ------------------------------------------------

MuteObjCrystUserInfo::MuteObjCrystUserInfo() :
    msave_info_func(ObjCryst::fpObjCrystInformUser)
{
    ObjCryst::fpObjCrystInformUser = black_hole_info;
}


MuteObjCrystUserInfo::~MuteObjCrystUserInfo()
{
    this->release();
}


void MuteObjCrystUserInfo::release()
{
    using ObjCryst::fpObjCrystInformUser;
    if (msave_info_func)  fpObjCrystInformUser = msave_info_func;
    msave_info_func = NULL;
}

// class CaptureStdOut -------------------------------------------------------

CaptureStdOut::CaptureStdOut() :
    msave_cout_buffer(std::cout.rdbuf())
{
    std::cout.rdbuf(moutput.rdbuf());
}


CaptureStdOut::~CaptureStdOut()
{
    this->release();
}


std::string CaptureStdOut::str() const
{
    return moutput.str();
}


void CaptureStdOut::release()
{
    if (msave_cout_buffer)  std::cout.rdbuf(msave_cout_buffer);
    msave_cout_buffer = NULL;
}

// free functions ------------------------------------------------------------

void assignCrystVector(CrystVector<double>& cv, bp::object obj)
{
    // copy data directly if it is a numpy array of doubles
    PyArrayObject* a = PyArray_Check(obj.ptr()) ?
        reinterpret_cast<PyArrayObject*>(obj.ptr()) : NULL;
    bool isdoublenumpyarray = a &&
        (1 == PyArray_NDIM(a)) &&
        (NPY_DOUBLE == PyArray_TYPE(a));
    if (isdoublenumpyarray)
    {
        const double* src = static_cast<double*>(PyArray_DATA(a));
        npy_intp stride = PyArray_STRIDE(a, 0) / PyArray_ITEMSIZE(a);
        cv.resize(PyArray_SIZE(a));
        double* dst = cv.data();
        const double* last = dst + cv.size();
        for (; dst != last; ++dst, src += stride)  *dst = *src;
    }
    // otherwise copy elementwise converting each element to a double
    else
    {
        bp::stl_input_iterator<double> begin(obj), end;
        // use intermediate list to preserve cv when conversion fails.
        std::list<double> values(begin, end);
        cv.resize(values.size());
        std::list<double>::const_iterator vv = values.begin();
        double* dst = cv.data();
        for (; vv != values.end(); ++vv, ++dst)  *dst = *vv;
    }
}


int check_index(int idx, int size, NegativeIndexFlag nflag)
{
    const int lo = (nflag == ALLOW_NEGATIVE) ? -size : 0;
    if (idx < lo || idx >= size)
    {
        PyErr_SetString(PyExc_IndexError, "Index out of range.");
        bp::throw_error_already_set();
    }
    int rv = (idx < 0) ? size + idx : idx;
    return rv;
}
