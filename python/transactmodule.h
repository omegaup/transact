#define STATIC_ASSERT(cond) \
	extern char (*STATIC_ASSERT(void)) [sizeof(char[1 - 2*!(cond)])]

struct message {
	ptrdiff_t next;
	size_t size;
	size_t free;
	size_t msgid;
	char data[32];
};

STATIC_ASSERT(sizeof(struct message) == 64);

struct message_root {
	volatile ptrdiff_t current_msg_offset;
	volatile ptrdiff_t free_offset;
	volatile ptrdiff_t small_message_list;
	volatile ptrdiff_t large_message_list;
	char padding[32];

	struct message root[0];
};

STATIC_ASSERT(sizeof(struct message_root) == 64);

typedef struct {
	PyObject_HEAD
	int transact_fd;
	int shm_fd;
	PyObject* name;
	size_t size;
	struct message_root* shm;
} Interface;

typedef struct {
	PyObject_HEAD
	unsigned int msgid;
	ptrdiff_t offset;
	char* writeptr;
	char* readptr;
	char* end;
	struct message* message;
} Message;

static void
Interface_dealloc(Interface* self);
static PyObject*
Interface_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int
Interface_init(Interface* self, PyObject* args, PyObject* kwds);
static PyObject*
Interface_allocate(Interface* self, PyObject* args);
static PyObject*
Interface_call(Interface* self, PyObject* args);
static PyObject*
Interface_get(Interface* self, PyObject* args);

static void
Message_dealloc(Message* self);
static PyObject*
Message_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int
Message_init(Message* self, PyObject* args, PyObject* kwds);
static PyObject*
Message_read(Message* self, PyObject* args);
static PyObject*
Message_write(Message* self, PyObject* args);
