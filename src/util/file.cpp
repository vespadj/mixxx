#include "util/file.h"

MFile::MFile()
        : m_pSecurityToken(NULL) {
}

MFile::MFile(const QString& file)
        : m_fileName(file),
          m_file(file),
          m_pSecurityToken(Sandbox::openSecurityToken(m_file, true)) {
}

MFile::MFile(const MFile& other)
        : m_fileName(other.m_fileName),
          m_file(m_fileName),
          m_pSecurityToken(Sandbox::openSecurityToken(m_file, true)) {
}

MFile::~MFile() {
    Sandbox::closeSecurityToken(m_pSecurityToken);
}

MFile& MFile::operator=(const MFile& other) {
    Sandbox::closeSecurityToken(m_pSecurityToken);
    m_fileName = other.m_fileName;
    m_file.setFileName(m_fileName);
    m_pSecurityToken = Sandbox::openSecurityToken(m_file, true);
    return *this;
}

MDir::MDir()
        : m_pSecurityToken(NULL) {
}

MDir::MDir(const QString& path)
        : m_dirPath(path),
          m_dir(path),
          m_pSecurityToken(Sandbox::openSecurityToken(m_dir, true)) {
}

MDir::MDir(const MDir& other)
        : m_dirPath(other.m_dirPath),
          m_dir(m_dirPath),
          m_pSecurityToken(Sandbox::openSecurityToken(m_dir, true)) {
}

MDir::~MDir() {
    Sandbox::closeSecurityToken(m_pSecurityToken);
}

MDir& MDir::operator=(const MDir& other) {
    Sandbox::closeSecurityToken(m_pSecurityToken);
    m_dirPath = other.m_dirPath;
    m_dir = QDir(m_dirPath);
    m_pSecurityToken = Sandbox::openSecurityToken(m_dir, true);
    return *this;
}
