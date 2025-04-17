package it.unipg;

import java.awt.Dimension;
import java.awt.Graphics;
import javax.swing.JFrame;

public class DrawingWindow extends JFrame {

	public DrawingWindow() {
		super("DrawingWindow");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setPreferredSize(new Dimension(800, 600));
		pack();
		JFrame frame = new JFrame();
		
	}

	@Override
	public void paint(Graphics g) {
		super.paint(g);
		g.drawRect(0, 0, 200, 100);

	}

	public static void main(String[] args) {
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				new DrawingWindow().setVisible(true);
			}
		});

	}

} // end class
