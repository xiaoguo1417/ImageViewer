#include "ImageViewer.h"

ImageViewer::ImageViewer(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

	m_scale = 1.0;
	m_rotation = 0.0;
	m_scaleBasePt = QPointF(0, 0);
	m_buttonLeftPressed = false;
	m_buttonMiddlePressed = false;
	isShotting = false;
	setMouseTracking(true);//ʵ��mouseMoveEvent
	ui.centralWidget->setMouseTracking(true);
	ui.widget->setMouseTracking(true);

	sizeLabel->setMinimumSize(150, 20);
	posLabel->setMinimumSize(150, 20);
	valueLabel->setMinimumSize(150, 20);
	versionLabel->setText("v1.1.0 (Mar 16 2022)");//�汾��Ϣ
	ui.statusBar->addPermanentWidget(versionLabel);
	sizeLabel->setText("size");
	sizeLabel->setAlignment(Qt::AlignCenter);
	ui.statusBar->addWidget(sizeLabel);
	posLabel->setText("pos");
	posLabel->setAlignment(Qt::AlignCenter);
	ui.statusBar->addWidget(posLabel);
	valueLabel->setText("grayValue");
	valueLabel->setAlignment(Qt::AlignCenter);
	ui.statusBar->addWidget(valueLabel);

	ui.widget->installEventFilter(this);//�¼�������

	filename = "lena.bmp";
	m_img.load(filename);

	connect(ui.action_open, &QAction::triggered, this, &ImageViewer::open);
	connect(ui.action_shot, &QAction::triggered, this, &ImageViewer::shot);
	connect(m_shot, SIGNAL(endShot()), this, SLOT(endShotting()));
	connect(m_shot, SIGNAL(savePic()), this, SLOT(savePicture()));
	connect(m_shot, SIGNAL(reset()), this, SLOT(resetShot()));
	connect(m_shot, SIGNAL(sig_modify(QRect)), this, SLOT(modifyShot(QRect)));
}

bool ImageViewer::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == ui.widget && event->type() == QEvent::Paint)//�����¼�
	{
		myPaintEvent();
		return true;
	}
	return QWidget::eventFilter(watched, event);
}

void ImageViewer::open()
{
	filename = QFileDialog::getOpenFileName(this,
		tr("Select picture to open"),
		"",
		tr("Images (*.png *.bmp *.jpg *.tif *.GIF )"));//�ɴ򿪵��ļ�����
	m_img.load(filename);
}

void ImageViewer::myPaintEvent()
{
	QPainter p(ui.widget);
	p.fillRect(ui.widget->rect(), Qt::gray);
	p.save();

	QTransform transform = getTransform(false);
	p.setWorldTransform(transform);
	p.drawImage(QPoint(0, 0), m_img);
	p.restore();//���ص�saveʱ��״̬����Ĭ������ϵ״̬

	//QPen mypen;
	//mypen.setWidth(1);                     // 1 ��ʾ��Ĵ�С����״Ϊ���Σ�
	//mypen.setColor(Qt::black);
	//p.setPen(mypen);
	//p.drawPoint(0, 0);

	if (isShotting)
	{
		QRectF w_rect = QRectF(w_ltPoint, w_rbPoint);
		//����
		QPen pen;
		pen.setBrush(Qt::red);
		pen.setStyle(Qt::DashLine);
		pen.setWidth(3);
		p.setPen(pen);
		p.drawRect(w_rect);
	}

	if (m_scale > 30)
	{
		//1.����ӳ��ڵ�����
		QPointF tl = screenToWorld(QPointF(ui.widget->x(), ui.widget->y()), false);
		QPointF br = screenToWorld(QPointF(ui.widget->size().width(), ui.widget->size().height()), false);
		QRect rectInView(tl.toPoint(), br.toPoint());

		QPen pen(Qt::gray);
		p.setPen(pen);
		QVector<QLineF> lines = getPixelBoundary(rectInView);
		p.drawLines(lines);
		//p.restore();
	}

	if (m_scale > 48)
	{
		//qDebug() << "paint value!";
		//1.����ӳ��ڵ�����
		QPointF tl = screenToWorld(QPointF(ui.widget->x(), ui.widget->y()), false);
		QPointF br = screenToWorld(QPointF(ui.widget->size().width(), ui.widget->size().height()), false);

		//TODO����ȡÿ���������ֵ
		QRect rectInView(tl.toPoint(), br.toPoint());
		QVector<QPointF> pointsOnScreen;
		QVector<QString> valueString;

		for (int y = rectInView.y(); y < rectInView.y() + rectInView.height(); ++y)
		{
			for (int x = rectInView.x(); x < rectInView.x() + rectInView.width(); ++x)
			{
				QRgb pixValue = m_img.pixel(x, y);
				QPointF pointOnScreen = worldToScreen(QPointF(x + 0.5, y + 0.5));
				pointsOnScreen.push_back(pointOnScreen);
				valueString.push_back(QString::number(qGray(pixValue)));
			}

		}

		//��������ֵ
		QPen pen(Qt::gray);
		p.setPen(pen);
		QFont font;
		font.setFamily("Times");
		font.setPointSize(8);
		p.setFont(font);
		for (int i = 0; i < pointsOnScreen.size(); ++i)
		{
			int strWidth = p.fontMetrics().width(valueString[i]);
			int strHeight = p.fontMetrics().height();
			QPointF fontPoint =
				QPointF(pointsOnScreen[i]
					- QPointF(strWidth / 2.0, -strHeight / 4.0));
			QRect fontRect(fontPoint.x() - 1,
				fontPoint.y() - strHeight + strHeight / 8,
				strWidth + 2, strHeight + +strHeight / 8);
			p.setPen(Qt::gray);
			p.setBrush(QBrush(Qt::gray));
			p.drawRect(fontRect);
			p.setPen(Qt::black);
			p.drawText(fontPoint, valueString[i]);
		}
	}

	if (m_img.isNull())
	{
		return;
	}
	QString size = QString::number(m_img.width()) + "*" + QString::number(m_img.height());
	sizeLabel->setText(size);
	ui.statusBar->addWidget(sizeLabel);
}

QTransform ImageViewer::getTransform(bool includeRotation/* = true*/)
{
	QTransform transform;
	if (!m_img.isNull())
	{
		transform.translate(m_scaleBasePt.x(), m_scaleBasePt.y());//����ԭ������¶��壬����Ϊ���¶����x���꣬y����

		transform.scale(m_scale, m_scale);
		if (includeRotation)
		{
			QPointF c(m_img.width() / qreal(2), m_img.height() / qreal(2));
			transform.translate(c.x(), c.y());
			transform.rotate(m_rotation);
			transform.translate(-c.x(), -c.y());
		}

	}
	return transform;
}

//��ͼ�����굽�������ϵ��
QPointF ImageViewer::worldToScreen(QPointF &pt, bool includeRotation/* = true*/)
{
	QTransform t = getTransform(includeRotation);
	return t.map(pt);//���ر仯��ĵ�
}

//��ȡת�����󲢽���ת�������������굽ͼ������ϵ������
QPointF ImageViewer::screenToWorld(QPointF &pt, bool includeRotation/* = true*/)
{
	QTransform t = getTransform(includeRotation);
	QTransform t1 = t.inverted();//�����
	return t1.map(pt);//��ʾ�ѵ�ʹ�ñ任����matrix����ת��
}

//�߽���
QVector<QLineF> ImageViewer::getPixelBoundary(QRect &rect)
{
	QVector<QLineF> lines;
	QPointF left, right, top, buttom;
	for (int y = rect.y(); y < rect.y() + rect.height(); ++y)
	{
		left = worldToScreen(QPointF(rect.x() - 1, y));
		right = worldToScreen(QPointF(rect.x() + rect.width() + 1, y));
		lines.append(QLineF(left, right));
	}
	for (int x = rect.x(); x < rect.x() + rect.width(); ++x)
	{
		top = worldToScreen(QPointF(x, rect.y() - 1));
		buttom = worldToScreen(QPointF(x, rect.y() + rect.height() + 1));
		lines.append(QLineF(top, buttom));
	}
	return lines;
}

void ImageViewer::mouseMoveEvent(QMouseEvent * event)
{
	if (m_buttonLeftPressed & isShotting)
	{
		QPointF curPt = event->pos();
		w_rbPoint = QPoint(curPt.x() - 13, curPt.y() - 89);
		QPointF pt2 = screenToWorld(w_rbPoint, false);
		rbPoint = pt2;
		
		/*QPointF pt1 = screenToWorld(m_lastPressedMousePt, false);
		QPointF pt2 = screenToWorld(curPt, false);

		m_selection = QRect(QPoint(qMin(pt1.x(), pt2.x()), qMin(pt1.y(), pt2.y())), QPoint(qMax(pt1.x(), pt2.x()), qMax(pt1.y(), pt2.y())));
		m_lastPressedMousePt = event->pos();*/
		//repaint();
	}
	else if (m_buttonMiddlePressed)
	{
		QPointF mv = event->pos() - m_lastPressedMousePt;
		m_scaleBasePt += mv;
		m_lastPressedMousePt = event->pos();
		repaint();
	}
	if (!m_img.isNull())
	{
		QPointF curPt = event->pos();
		curPt = QPoint(curPt.x() - 13, curPt.y() - 89);//����ԭ�㲻��widget���Ͻ�,13,89
		QPointF pt2 = screenToWorld(curPt, false);
		
		int x = pt2.x();
		curWPt = pt2;
		
		int y = pt2.y();
		QRgb pixValue = m_img.pixel(x, y);
		QString str = "[" + QString::number(x) + "," +
			QString::number(y) + "]---" +
			QString::number(qGray(pixValue));
		QString size = QString::number(m_img.width()) + "*" + QString::number(m_img.height());
		sizeLabel->setText(size);
		ui.statusBar->addWidget(sizeLabel);
		posLabel->setText(str);
		ui.statusBar->addWidget(posLabel);
		valueLabel->setText(QString::number(qGray(pixValue)));
		ui.statusBar->addWidget(valueLabel);
	}
}

void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton && isShotting) {
		m_buttonLeftPressed = true;
		QPointF curPt = event->pos();
		w_ltPoint = QPoint(curPt.x() - 13, curPt.y() - 89);//ת����widget���Ͻ�Ϊԭ��
		test = event->pos();
		QPointF pt1 = screenToWorld(w_ltPoint, false);
		ltPoint = pt1;
	}
	else if (event->button() == Qt::LeftButton && !isShotting)
		m_buttonMiddlePressed = true;
	//m_buttonLeftPressed = true;
	else if (event->button() == Qt::MidButton) {
		//m_buttonMiddlePressed = true;
		//����,�����ĵ�����
		/*QPointF curPt = event->pos();
		QPoint pt1 = QPoint(curPt.x() - 13, curPt.y() - 89);
		qDebug()<<pt1;*/
	}
	else if (event->button() == Qt::RightButton)
	{
		tlPt = curWPt;
		int x = tlPt.x();
		int y = tlPt.y();
		QString str = "[" + QString::number(x) + "," + QString::number(y) + "]";
		//ui.ptLEdit->setText(str);
	}
	m_lastPressedMousePt = event->pos();
}

void ImageViewer::mouseReleaseEvent(QMouseEvent * event)
{
	m_buttonLeftPressed = false;
	m_buttonMiddlePressed = false;
	//isShotting = false;
	if (event->button() == Qt::LeftButton && isShotting)
	{
		
		m_selection = QRect(QPoint(qMin(ltPoint.x(), rbPoint.x()), qMin(ltPoint.y(), rbPoint.y())), QPoint(qMax(ltPoint.x(), rbPoint.x()), qMax(ltPoint.y(), rbPoint.y())));
		//qDebug() << m_selection;
		m_shot->setRect(m_selection);
		QRectF w_rect = QRectF(w_ltPoint, w_rbPoint);
		repaint();
	}
	if (event->button() == Qt::RightButton)
	{
		m_selection = QRect();
		m_refined = QRect();
		repaint();
		/*myPaintEvent();*/
	}
}

void ImageViewer::wheelEvent(QWheelEvent *event)//��ȡÿ�α任��ԭ���λ�ü��Ŵ���
{
	if (!m_img.isNull())
	{
		QPoint angle = event->angleDelta();//���ع��ֶ���������,��ˮƽ�ʹ�ֱ����

		if (event->modifiers() == Qt::ShiftModifier)//shiftΪ������������ת
		{
			m_rotation += angle.y() / 8;
		}
		else
		{
			QPointF pt1 = event->posF();
			//pt1 = QPoint(pt1.x(), pt1.y());
			pt1 = QPoint(pt1.x() - 13, pt1.y() - 89);//��������ԭ�㲻��widget���Ͻǵ�����
			QPointF p = worldToScreen(QPointF(0, 0));//ͼ���(0,0)���Ӧ����Ļ���꣬�ҵ���ǰͼ�����Ͻǵ��ڳ�ʼ����ϵ�µ�����ֵ

			QPointF pt2;
			if (angle.y() > 0)
			{
				pt2 = pt1 + (p - pt1)*0.618;//��һ�·Ŵ�0.618
				m_scale *= 0.618;
			}
			else
			{
				pt2 = pt1 + (p - pt1) / 0.618;
				m_scale /= 0.618;
			}

			QTransform rotationTransform;
			QPointF c(m_img.width() / qreal(2), m_img.height() / qreal(2));
			rotationTransform.translate(c.x(), c.y());
			rotationTransform.rotate(m_rotation);
			rotationTransform.translate(-c.x(), -c.y());

			QTransform scalingTransform;
			scalingTransform.scale(m_scale, m_scale);

			QPointF a3 = (scalingTransform.map(rotationTransform.map(QPointF(0, 0)))); //ok

			QPointF pt3 = pt2 - a3;
			m_scaleBasePt = pt3;
		}
		repaint();
	}
}

void ImageViewer::shot()
{
	isShotting = true;

	m_shot->setParent(this);//ָ��������
	m_shot->setWindowFlags(m_shot->windowFlags() | Qt::Dialog);
	QPoint dis = QPoint(this->geometry().x() + 10, this->geometry().y() +this->height() - m_shot->height() - 38);
	m_shot->move(dis);//move��ԭ���Ǹ����ڵ����Ͻǣ�  ���û�и����ڣ������漴Ϊ������
	
	m_shot->show();
	
}

void ImageViewer::endShotting()
{
	isShotting = false;
}

void ImageViewer::savePicture()
{
	QString filename1 = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("Images (*.png *.bmp *.jpg)")); //ѡ��·��
	QImage shotted = m_img.copy(m_selection);
	shotted.save(filename1);
}

void ImageViewer::resetShot()
{
	w_ltPoint = QPoint(0, 0);
	w_rbPoint = QPoint(0, 0);
	m_selection = QRect(0, 0, 0, 0);
	ltPoint = QPointF(0, 0);
	rbPoint = QPointF(0, 0);
	m_shot->setRect(m_selection);
	repaint();
}

void ImageViewer::modifyShot(QRect rect)
{
	ltPoint = rect.topLeft();
	rbPoint = rect.bottomRight();
	m_selection = QRect(QPoint(qMin(ltPoint.x(), rbPoint.x()), qMin(ltPoint.y(), rbPoint.y())), QPoint(qMax(ltPoint.x(), rbPoint.x()), qMax(ltPoint.y(), rbPoint.y())));

	QPointF temp = worldToScreen(ltPoint, false);
	w_ltPoint = temp;
	QPointF temp2 = worldToScreen(rbPoint, false);
	w_rbPoint = temp2;

	repaint();
}
