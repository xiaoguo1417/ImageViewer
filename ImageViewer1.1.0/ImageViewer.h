#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ImageViewer.h"
#include <QPainter>
#include <QPaintEvent>
#include <QImage>
#include <QFileDialog>
#include <QLabel>
#include "ImageShot.h"
#include <QDebug>
#include <QtCore>

class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer(QWidget *parent = Q_NULLPTR);
	bool eventFilter(QObject *watched, QEvent *event);
	QTransform getTransform(bool includeRotation);
	QVector<QLineF> getPixelBoundary(QRect &rect);
	QPointF worldToScreen(QPointF &pt, bool includeRotation = true);
	QPointF screenToWorld(QPointF &pt, bool includeRotation/* = true*/);

protected:
	/*virtual void myPaintEvent(QPaintEvent * event);*/
	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual void wheelEvent(QWheelEvent *event);

private:
    Ui::ImageViewerClass ui;
	void myPaintEvent();

	QImage m_img;//全局图片
	QString filename;//图片位置,name
	qreal m_scale;
	qreal m_rotation;
	QPointF m_scaleBasePt;
	QPointF m_lastPressedMousePt;
	QRect m_selection;
	QRect m_refined;

	QPointF curWPt;//当前点在图像中的坐标
	QPointF tlPt;//矩形区域的左上角点

	bool m_buttonLeftPressed;
	bool m_buttonMiddlePressed;
	bool isShotting;

	QLabel *sizeLabel = new QLabel(this);
	QLabel *posLabel = new QLabel(this);
	QLabel *valueLabel = new QLabel(this);
	QLabel *versionLabel = new QLabel(this);

	ImageShot *m_shot = new ImageShot();
	QPointF ltPoint;
	QPointF rbPoint;
	QPointF w_ltPoint;
	QPointF w_rbPoint;
	QPoint test;

	private slots:
	void open();
	void shot();
	void endShotting();
	void savePicture();
	void resetShot();
	void modifyShot(QRect rect);

signals:
	
};
