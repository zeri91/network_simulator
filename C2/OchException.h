#ifndef OCHEXCEPTION_H
#define OCHEXCEPTION_H

namespace NS_OCH {

class OchException {
public:
	static char m_pErrorMsg[1025];
};

#define THROW_OCH_EXEC(pMsg)	{ sprintf(OchException::m_pErrorMsg, "- Exception: %s:%s: %s", __FILE__, __LINE__, pMsg); throw OchException(); }

};
#endif
