#ifndef __TRANSACT_H
#define __TRANSACT_H

struct classes {
	jclass interface;
	jclass message;
};

extern struct classes g_classes;

struct fields {
	jfieldID interface_interfacePtr;
	jfieldID message_messagePtr;
	jfieldID message_msgid;
};

extern struct fields g_fields;

struct methods {
};

extern struct methods g_methods;

#endif  // __TRANSACT_H
