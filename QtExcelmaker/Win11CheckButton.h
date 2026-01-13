#pragma once

#include <QWidget> //所有可视控件的基类，提供事件、绘制和布局的基础。
#include <QVBoxLayout> //垂直方向的布局管理器，用来按列排列子控件
#include <QCheckBox> //复选框控件，允许用户在选中和未选中状态之间切换
#include <QPainter> //提供绘图功能的类，用于在控件上绘制图形和文本
#include <QPropertyAnimation> //属性动画类，可对 QObject 派生对象的属性做插值动画（如位置、透明度）。
#include <QPainterPath> //表示矢量路径的容器，用于描述复杂形状（直线、曲线、填充区域等），通常配合 QPainter 绘制
#include "DesignSystem.h"

class Win11CheckButton : public QCheckBox
{
	Q_OBJECT
		Q_PROPERTY(qreal progress READ progress WRITE setProgress)
public:
	Win11CheckButton(QWidget* parent);
	~Win11CheckButton();

	qreal progress() const { return m_progress; }
	void setProgress(qreal p) {
		m_progress = p;
		update();
	}
protected:
	void paintEvent(QPaintEvent* event) override;
	QSize sizeHint() const;
private:
	qreal approximatePathLength(const QPainterPath& path);
	QPainterPath strokePathPortion(const QPainterPath& path, qreal length);
private:
	qreal m_progress;
	QPropertyAnimation* m_anim;
	QColor themeColor = DesignSystem::instance()->primaryColor();

	// 可调属性
	int boxSize = 16;
	int boxRadius = 3;
	int boxSpacing = 6;
	int boxMargin = 4;
};