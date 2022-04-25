# ImageViewer记录

## 1.概述

ImageViewer V1.0.0实现图片的显示，==放大缩小==，鼠标点处图像==坐标值和灰度值显示==以及自定义大小==矩形区域的截取==，主要参考为github项目[MyImageViewer](https://github.com/liuchang0523/MyImageViewer/tree/master/MyImageViewer)，主要记录事件的重写，子控件上绘图以及坐标转换等过程。

主要功能展示：

1. 图片的放大缩小展示，以及鼠标位置处像素信息在状态栏的显示
 ![1646226638146](C:\Users\xiaoguo\AppData\Roaming\Typora\typora-user-images\1646226638146.png)

2. 鼠标右键选取矩形区域左上角点，文本框输入相应宽和高进行预览
![1646226802076](C:\Users\xiaoguo\AppData\Roaming\Typora\typora-user-images\1646226802076.png)

## 2.事件重写

自定义事件需要重写事件函数

```C++
protected:
	virtual void paintEvent(QPaintEvent * event);
	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual void wheelEvent(QWheelEvent *event);
```

在mainWindow中鼠标移动但mouseMoveEvent不响应：要先把 QMainWindow 的 CentrolWidget 使用 setMouseTracking(true) 开启移动监视。然后再把 QMainWindow 的 setMouseTracking(true) 开启监视。之后就一切正常了。

```C++
setMouseTracking(true);//实现mouseMoveEvent
ui.centralWidget->setMouseTracking(true);
ui.widget->setMouseTracking(true);
```

滚轮事件对应操作，利用QWheelEvent的delta()函数获得滚轮滚动的距离值，通过此值判断滚轮滚动的方向。若delta大于0，则表示滚轮向前（远离用户的方向），小于零则表明向后，（靠近用户的方向）delta(),新版的Qt已经将其废除，它的返回值相当于angleDelta()的返回值的y点坐标；鼠标的滚动事件，滚轮每滚动1度，相当于移动了8度

```C++
QPoint angle = event->angleDelta();//返回滚轮度数的增量,有水平和垂直滚轮
angle.y();//滚动的距离
event->modifiers() == Qt::ShiftModifier //判断辅助键是否按下
event->posF()；//鼠标的位置

```

## 3.子控件绘图

直接重写paintEvent()方法在MainWindow中进行绘图，即使加了Widget等子控件，绘图区域还是会默认为centerWidget的整个区域，导致图片放大后会超出子控件，这时就需要在子空间中绘图，主要有下面两种方法，[参考博客](https://www.cnblogs.com/kang-l/p/8387584.html)：

1. 重写子窗口的控件类（即继承该类，并重载其paintEvent()方法），实现其paintEvent()方法，然后在ui里面将原来的控件提升(promote to)为新类，参考的[MyImageViewer](https://github.com/liuchang0523/MyImageViewer/tree/master/MyImageViewer)就是使用这种方法，本项目没有使用

2. 因为Qt存在事件过滤机制，事件过滤器会将子控件上的绘图事件过滤，因此无法在子控件上使用QPainter进行绘图。如果需要在子控件上使用QPainter，同样需要事件过滤器。使用事件过滤器，重写eventFilter()，在子窗口或控件中注册事件过滤器（installEventFilter()），然后就可以在eventFilter()里随心所欲地重写过滤到的paintEvent()事件了。使用步骤：安装事件过滤器： ui->paint_widget（子控件名，可更换为需要使用的控件）->installEventFilter(this);实现在过滤器中指定的操作

   ```C++
   ui.widget->installEventFilter(this);//安装事件过滤器
   
   bool test0224::eventFilter(QObject *watched, QEvent *event)
   {
   	if (watched == ui.widget && event->type() == QEvent::Paint)
   	{
   		myPaintEvent();//重写的子控件画图函数
   		return true;
   	}
   	return QWidget::eventFilter(watched, event);
   }
   ```

