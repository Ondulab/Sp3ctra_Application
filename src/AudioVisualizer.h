#ifndef AUDIOVISUALIZER_H
#define AUDIOVISUALIZER_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <vector>

class AudioVisualizer : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  AudioVisualizer(QWidget *parent = nullptr);
  ~AudioVisualizer();

  void updateData(const unsigned char *r, const unsigned char *g,
                  const unsigned char *b, int size);
  void updateDataWithScroll(const unsigned char *r, const unsigned char *g,
                            const unsigned char *b, int size);

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

private:
  void setupShaders();
  void setupBuffers();

  QOpenGLShaderProgram m_program;
  QOpenGLBuffer m_vbo;
  QOpenGLVertexArrayObject m_vao;

  std::vector<float> m_vertices;
  std::vector<unsigned char> m_r;
  std::vector<unsigned char> m_g;
  std::vector<unsigned char> m_b;

  // Historique pour le défilement
  std::vector<std::vector<unsigned char>> m_historyR;
  std::vector<std::vector<unsigned char>> m_historyG;
  std::vector<std::vector<unsigned char>> m_historyB;
  int m_historySize;

  int m_dataSize;
  bool m_dataUpdated;
};

#endif // AUDIOVISUALIZER_H
