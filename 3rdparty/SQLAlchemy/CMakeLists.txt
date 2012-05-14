configure_file(
  SQLAlchemy-0.7.7.zip
  ${CMAKE_BINARY_DIR}/pythonlibs/SQLAlchemy-0.7.7.zip
  COPYONLY
)

install(
  FILES SQLAlchemy-0.7.7.zip
  DESTINATION share/clementine/pythonlibs/
)
