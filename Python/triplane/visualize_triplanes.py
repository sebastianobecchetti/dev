import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def visualize_triplanes(num_points=100, bounds=(-1, 1)):
    """
    Visualizes 3D points and their projections onto triplanes (XY, XZ, YZ).
    
    Args:
        num_points (int): Number of random points to generate.
        bounds (tuple): The (min, max) bounds for the 3D space.
    """
    
    # 1. Generate random 3D points
    min_val, max_val = bounds
    points = np.random.uniform(min_val, max_val, size=(num_points, 3))
    
    x = points[:, 0]
    y = points[:, 1]
    z = points[:, 2]

    # 2. Setup the plot with subplots
    fig = plt.figure(figsize=(18, 12))
    
    # Subplot 1: 3D View
    ax_3d = fig.add_subplot(221, projection='3d')
    ax_3d.set_title("3D View & Projectios")
    ax_3d.set_xlim(min_val, max_val)
    ax_3d.set_ylim(min_val, max_val)
    ax_3d.set_zlim(min_val, max_val)
    ax_3d.set_xlabel("X")
    ax_3d.set_ylabel("Y")
    ax_3d.set_zlabel("Z")
    
    # Plot original 3D points
    ax_3d.scatter(x, y, z, c='blue', marker='o', alpha=0.6, label='Points')
    
    # Add projections to 3D view (optional, keeping for reference)
    ax_3d.scatter(x, y, np.full_like(z, min_val), c='red', marker='s', alpha=0.1, label='XY Proj')
    ax_3d.scatter(x, np.full_like(y, max_val), z, c='green', marker='s', alpha=0.1, label='XZ Proj')
    ax_3d.scatter(np.full_like(x, min_val), y, z, c='orange', marker='s', alpha=0.1, label='YZ Proj')
    ax_3d.legend()

    # Subplot 2: XY Plane (Top-Down)
    ax_xy = fig.add_subplot(222)
    ax_xy.set_title("XY Plane Projection (Z ignored)")
    ax_xy.scatter(x, y, c='red', alpha=0.6)
    ax_xy.set_xlabel("X")
    ax_xy.set_ylabel("Y")
    ax_xy.set_xlim(min_val, max_val)
    ax_xy.set_ylim(min_val, max_val)
    ax_xy.grid(True)

    # Subplot 3: XZ Plane (Front View)
    ax_xz = fig.add_subplot(223)
    ax_xz.set_title("XZ Plane Projection (Y ignored)")
    ax_xz.scatter(x, z, c='green', alpha=0.6)
    ax_xz.set_xlabel("X")
    ax_xz.set_ylabel("Z")
    ax_xz.set_xlim(min_val, max_val)
    ax_xz.set_ylim(min_val, max_val)
    ax_xz.grid(True)

    # Subplot 4: YZ Plane (Side View)
    ax_yz = fig.add_subplot(224)
    ax_yz.set_title("YZ Plane Projection (X ignored)")
    ax_yz.scatter(y, z, c='orange', alpha=0.6)
    ax_yz.set_xlabel("Y")
    ax_yz.set_ylabel("Z")
    ax_yz.set_xlim(min_val, max_val)
    ax_yz.set_ylim(min_val, max_val)
    ax_yz.grid(True)

    # Adjust layout
    plt.tight_layout()
    
    # Save the figure
    output_filename = "triplane_visualization_combined.png"
    plt.savefig(output_filename, dpi=300)
    print(f"Visualization saved to {output_filename}")
    # plt.show() # Uncomment if running locally with a display

if __name__ == "__main__":
    visualize_triplanes(num_points=50)
