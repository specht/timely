include(../base.pro)

TARGET = timely
CONFIG(debug, debug|release) {
	TARGET = $$join(TARGET,,,_debug)
}
