package it.unipg;

import javax.swing.JFrame;
import java.awt.Dimension;
import java.awt.Color;
import java.awt.Graphics;

public class DrawingWindow extends JFrame {

	public DrawingWindow() {
		super("DrawingWindow");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setPreferredSize(new Dimension(1920, 1080));
		pack();
	}

	@Override
	public void paint(Graphics g) {
		super.paint(g);
		// add code to draw a house
		//
		g.setColor(new Color(255, 0, 0));
		g.fillRect(100, 100, 200, 400);
		System.out.println(g.getColor());
	}

	public static void main(String[] args) {
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				new DrawingWindow().setVisible(true);
			}
		});
	}

} // end class main
