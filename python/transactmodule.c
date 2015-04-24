#include <Python.h>
#include "structmember.h"
#include "transactmodule.h"

#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

static PyMemberDef Interface_members[] = {
	{"name", T_OBJECT_EX, offsetof(Interface, name), 0, "name of the interface"},
	{NULL} // Sentinel
};

static PyMethodDef Interface_methods[] = {
	{"allocate", (PyCFunction)Interface_allocate, METH_VARARGS,
		"Allocates a Message with the specified size in bytes"},
	{"call", (PyCFunction)Interface_call, METH_VARARGS,
		"Performs an IPC call. The response will be in the message parameter"},
	{"get", (PyCFunction)Interface_get, METH_VARARGS,
		"Waits until the other process has posted a message"},
	{NULL} // Sentinel
};

static PyTypeObject InterfaceType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"transact.Interface",      /*tp_name*/
	sizeof(Interface),         /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)Interface_dealloc,/*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"A representation of a transact interface",           /* tp_doc */
	0,		                     /* tp_traverse */
	0,		                     /* tp_clear */
	0,		                     /* tp_richcompare */
	0,		                     /* tp_weaklistoffset */
	0,		                     /* tp_iter */
	0,		                     /* tp_iternext */
	Interface_methods,         /* tp_methods */
	Interface_members,         /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Interface_init,  /* tp_init */
	0,                         /* tp_alloc */
	Interface_new,             /* tp_new */
};

static PyMemberDef Message_members[] = {
	{"msgid", T_ULONG, offsetof(Message, msgid), 0, "message id"},
	{NULL} // Sentinel
};

static PyMethodDef Message_methods[] = {
	{"read", (PyCFunction)Message_read, METH_VARARGS,
		"reads |size| bytes from the message"},
	{"write", (PyCFunction)Message_write, METH_VARARGS,
		"writes |buf| into the message"},
	{NULL} // Sentinel
};

static PyTypeObject MessageType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"transact.Message",        /*tp_name*/
	sizeof(Message),           /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)Message_dealloc,/*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"A transact message",      /* tp_doc */
	0,		                     /* tp_traverse */
	0,		                     /* tp_clear */
	0,		                     /* tp_richcompare */
	0,		                     /* tp_weaklistoffset */
	0,		                     /* tp_iter */
	0,		                     /* tp_iternext */
	Message_methods,           /* tp_methods */
	Message_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Message_init,    /* tp_init */
	0,                         /* tp_alloc */
	Message_new,               /* tp_new */
};

static PyMethodDef TransactMethods[] = {
	{NULL, NULL, 0, NULL} // Sentinel
};

static void
Interface_dealloc(Interface* self) {
	Py_DECREF(self->name);
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject*
Interface_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	Interface* self;

	self = (Interface*)type->tp_alloc(type, 0);
	if (!self)
		return NULL;

	self->name = PyString_FromString("");
	if (!self->name) {
		Py_DECREF(self);
		return NULL;
	}

	return (PyObject*)self;
}

static int
Interface_init(Interface* self, PyObject* args, PyObject* kwds) {
	char parent;
	char *name, *transactName, *shmName;
	size_t size;

	if (!PyArg_ParseTuple(args, "bsssl", &parent, &name,
				&transactName, &shmName, &size)) {
		return -1;
	}

	Py_DECREF(self->name);
	self->name = PyString_FromString(name);
	if (!self->name) {
		PyErr_NoMemory();
		return -1;
	}

	self->size = size / sizeof(struct message_root);
	self->transact_fd = open(transactName, O_RDWR);
	if (self->transact_fd == -1) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, transactName);
		return -1;
	}

	// Make sure the child process waits until the parent issues a read() call.
	unsigned long long message = parent;
	if (write(self->transact_fd, &message, sizeof(message)) != sizeof(message)) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, transactName);
		return -1;
	}

	self->shm_fd = open(shmName, O_RDWR);
	if (self->shm_fd == -1) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, shmName);
		return -1;
	}
	if (parent && ftruncate(self->shm_fd, size) == -1) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, shmName);
		return -1;
	}
	self->shm = (struct message_root*)mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_SHARED, self->shm_fd, 0);
	if (self->shm == (struct message_root*)-1) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, shmName);
		return -1;
	}
	if (parent) {
		self->shm->free_offset = 0;
		self->shm->small_message_list = (ptrdiff_t)-1;
		self->shm->large_message_list = (ptrdiff_t)-1;
	}

	return 0;
}

static PyObject*
Interface_allocate(Interface* self, PyObject* args) {
	PyObject* obj;
	unsigned int msgid;
	ssize_t bytes;
	if (!PyArg_ParseTuple(args, "OIl", &obj, &msgid, &bytes)) {
		return NULL;
	}

	if (obj->ob_type != &MessageType) {
		PyErr_SetString(PyExc_TypeError, "must be Message");
		return NULL;
	}

	Message* msg = (Message*)obj;

	bytes += 32; // For the page header.
	bytes += (~(bytes - 1) & 0x3F); // Align to blocks.
	size_t blocks = bytes / sizeof(struct message);

	ptrdiff_t head = blocks == 1 ?
			self->shm->small_message_list :
			self->shm->large_message_list;
	ptrdiff_t next = head;
	ptrdiff_t prev_next = self->size + 1;

	// Try to reuse an old allocation.
	while (next != (ptrdiff_t)-1) {
		// The messages form a linked list that goes backwards. If any next pointer
		// lies outside of the legal values, or it does not appear before in the
		// shared memory region, it is definitely invalid.
		if (next < 0 || next >= self->size || next >= prev_next) {
			PyErr_SetString(PyExc_IOError, "Illegal next pointer");
			return NULL;
		}
		struct message* prev = self->shm->root + next;
		if (prev->free && prev->size == blocks) {
			prev->free = 0;

			msg->offset = next;
			msg->writeptr = msg->readptr = prev->data;
			msg->end = (char*)prev + blocks * sizeof(struct message);
			msg->message = prev;
			msg->msgid = msg->message->msgid = msgid;
			Py_RETURN_NONE;
		}
		prev_next = next;
		next = prev->next;
	}

	// Sanity check.
	ptrdiff_t free_offset = self->shm->free_offset;
	if (free_offset < 0 || free_offset >= self->size) {
		PyErr_SetString(PyExc_IOError, "Illegal free offset");
		return NULL;
	}

	// Need to perform allocation.
	if (self->size < free_offset + blocks) {
		// No more memory.
		return PyErr_Format(PyExc_IOError, "No more memory for arena allocation. "
				"Need shm size to be at least %zu\n",
				(free_offset + blocks) * sizeof(struct message) + sizeof(struct message_root));
	}

	struct message* ptr = self->shm->root + free_offset;
	ptr->next = head;
	ptr->size = blocks;
	ptr->free = 0;
	if (blocks == 1) {
		self->shm->small_message_list = free_offset;
	} else {
		self->shm->large_message_list = free_offset;
	}
	self->shm->free_offset = free_offset + blocks;

	msg->offset = free_offset;
	msg->writeptr = msg->readptr = ptr->data;
	msg->end = (char*)ptr + blocks * sizeof(struct message);
	msg->message = ptr;
	msg->msgid = msg->message->msgid = msgid;
	Py_RETURN_NONE;
}

static PyObject*
Interface_internalGet(Interface* self, Message* msg) {
	ptrdiff_t offset = self->shm->current_msg_offset;
	if (offset < 0 || offset >= self->size) {
		fprintf(stderr, "Illegal message offset\n");
		exit(1);
	}
	struct message* ptr = self->shm->root + offset;
	msg->offset = offset;
	msg->msgid = ptr->msgid;
	msg->writeptr = msg->readptr = ptr->data;
	msg->end = (char*)ptr + ptr->size * sizeof(struct message);
	msg->message = ptr;
	Py_RETURN_NONE;
}

static PyObject*
Interface_call(Interface* self, PyObject* args) {
	PyObject* obj;
	char noret, nofree;
	if (!PyArg_ParseTuple(args, "Obb", &obj, &noret, &nofree)) {
		return NULL;
	}

	if (obj->ob_type != &MessageType) {
		PyErr_SetString(PyExc_TypeError, "must be Message");
		return NULL;
	}

	Message* msg = (Message*)obj;

	self->shm->current_msg_offset = msg->offset;
	unsigned long long response;
	int ret = read(self->transact_fd, &response, sizeof(response));
	if (ret != sizeof(response)) {
		if (noret) {
			Py_Exit(0);
		}
		return PyErr_Format(PyExc_IOError, "%s died unexpectedly while calling 0x%x\n",
				PyString_AsString(self->name), msg->msgid);
	}
	if (!nofree) {
		msg->message->free = 1;
	}
	return Interface_internalGet(self, msg);
}

static PyObject*
Interface_get(Interface* self, PyObject* args) {
	PyObject* obj;
	if (!PyArg_ParseTuple(args, "O", &obj)) {
		return NULL;
	}

	if (obj->ob_type != &MessageType) {
		PyErr_SetString(PyExc_TypeError, "must be Message");
		return NULL;
	}

	return Interface_internalGet(self, (Message*)obj);
}

static void
Message_dealloc(Message* self) {
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject*
Message_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	Message* self = (Message*)type->tp_alloc(type, 0);
	return (PyObject*)self;
}

static int
Message_init(Message* self, PyObject* args, PyObject* kwds) {
	if (!PyArg_ParseTuple(args, "")) {
		return -1;
	}

	self->msgid = 0;
	self->writeptr = self->readptr = self->end = NULL;

	return 0;
}

static PyObject*
Message_read(Message* self, PyObject* args) {
	size_t size;
	if (!PyArg_ParseTuple(args, "K", &size)) {
		return NULL;
	}

	if (self->readptr + size > self->end) {
		PyErr_SetString(PyExc_IOError, "Invalid read size");
		return NULL;
	}

	PyObject* result = PyBuffer_FromMemory(self->readptr, size);
	self->readptr += size;
	return result;
}

static PyObject*
Message_write(Message* self, PyObject* args) {
	char* buf;
	int size;
	if (!PyArg_ParseTuple(args, "t#", &buf, &size)) {
		return NULL;
	}

	if (self->writeptr + size > self->end) {
		PyErr_SetString(PyExc_IOError, "Invalid write size");
		return NULL;
	}

	memcpy(self->writeptr, buf, size);
	self->writeptr += size;
	return PyInt_FromLong(size);
}

PyMODINIT_FUNC
inittransact(void) {
	PyObject *m;

	if (PyType_Ready(&InterfaceType) < 0)
		return;

	if (PyType_Ready(&MessageType) < 0)
		return;

	m = Py_InitModule("transact", TransactMethods);
	if (m == NULL)
		return;

	Py_INCREF(&InterfaceType);
	PyModule_AddObject(m, "Interface", (PyObject*)&InterfaceType);
	Py_INCREF(&MessageType);
	PyModule_AddObject(m, "Message", (PyObject*)&MessageType);
}
