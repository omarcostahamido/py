/* 

py/pyext - python external object for PD and MaxMSP

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"

// function table for module
PyMethodDef py::func_tbl[] = 
{
	{ "_send", py::py_send, METH_VARARGS,"Send message to a named object" },
#ifdef FLEXT_THREADS
	{ "_priority", py::py_priority, METH_VARARGS,"Set priority of current thread" },
#endif

	{ "_samplerate", py::py_samplerate, 0,"Get system sample rate" },
	{ "_blocksize", py::py_blocksize, 0,"Get system block size" },
	{ "_inchannels", py::py_inchannels, 0,"Get number of audio in channels" },
	{ "_outchannels", py::py_outchannels, 0,"Get number of audio out channels" },

	{NULL, NULL, 0, NULL} // sentinel
};

const C *py::py_doc =
	"py/pyext - python external object for PD and MaxMSP, (C)2002 Thomas Grill\n"
	"\n"
	"This is the pyext module. Available function:\n"
	"_send(args...): Send a message to a send symbol\n"
#ifdef FLEXT_THREADS
	"_priority(int): Raise/lower thread priority\n"
#endif
	"_samplerate(): Get system sample rate\n"
	"_blocksize(): Get current blocksize\n"
	"_inchannels(): Get number of audio in channels\n"
	"_outchannels(): Get number of audio out channels\n"
;



V py::tick(py *th)
{
	th->Lock();

	if(!th->thrcount) {
		// all threads have stopped
		th->shouldexit = false;
		th->stoptick = 0;
	}
	else {
		// still active threads 
		if(!--th->stoptick) {
			post("%s - Threads couldn't be stopped entirely - %i remaining",th->thisName(),th->thrcount);
			th->shouldexit = false;
		}
		else
			// continue waiting
			clock_delay(th->clk,PY_STOP_TICK);
	}

	th->Unlock();
}

V py::m_stop(int argc,t_atom *argv)
{
	if(thrcount) {
		Lock();

		I wait = PY_STOP_WAIT;
		if(argc >= 1 && CanbeInt(argv[0])) wait = GetAInt(argv[0]);

		I ticks = wait/PY_STOP_TICK;
		if(stoptick) {
			// already stopping
			if(ticks < stoptick) stoptick = ticks;
		}
		else
			stoptick = ticks;
		shouldexit = true;
		clock_delay(clk,PY_STOP_TICK);

		Unlock();
	}
		
}

PyObject *py::py_samplerate(PyObject *self,PyObject *args)
{
	return PyFloat_FromDouble(sys_getsr());
}

PyObject *py::py_blocksize(PyObject *self,PyObject *args)
{
	return PyLong_FromLong(sys_getblksize());
}

PyObject *py::py_inchannels(PyObject *self,PyObject *args)
{
#ifdef PD
	I ch = sys_get_inchannels();
#else // MAXMSP
	I ch = sys_getch(); // not functioning
#endif
	return PyLong_FromLong(ch);
}

PyObject *py::py_outchannels(PyObject *self,PyObject *args)
{
#ifdef PD
	I ch = sys_get_outchannels();
#else // MAXMSP
	I ch = sys_getch(); // not functioning
#endif
	return PyLong_FromLong(ch);
}

PyObject *py::py_send(PyObject *,PyObject *args)
{
    if(PyTuple_Check(args)) {
		PyObject *name = PyTuple_GetItem(args,0); // borrowed
		if(name && PyString_Check(name)) {
			const t_symbol *recv = MakeSymbol(PyString_AsString(name));
			
			PyObject *val = PyTuple_GetSlice(args,1,PyTuple_Size(args));  // new ref
			AtomList *lst = GetPyArgs(val);
			if(lst) {
				t_class **cl = (t_class **)GetBound(recv);
				if(cl) {
#ifdef PD
					pd_forwardmess(cl,lst->Count(),lst->Atoms());
#else
					#pragma message ("Send is not implemented")
#endif
				}
#ifdef _DEBUG
				else 
					post("py/pyext - Receiver doesn't exist");
#endif
			}
			else 
				post("py/pyext - No data to send");
			if(lst) delete lst;

			Py_DECREF(val);
		}
		else
			post("py/pyext - Send name is invalid");
	}

    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef FLEXT_THREADS
PyObject *py::py_priority(PyObject *self,PyObject *args)
{
	int val;
    if(!PyArg_ParseTuple(args, "i:py_priority", &val)) {
		post("py/pyext - Syntax: _priority [int]");
    }
	else
		ChangePriority(val);

    Py_INCREF(Py_None);
    return Py_None;
}
#endif


