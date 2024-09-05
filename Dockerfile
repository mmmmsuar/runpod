FROM gcc:10.2

# Set working directory
WORKDIR /app

# Copy the necessary files
COPY keygen.c handler.py rclone.conf start.sh /app/

# Make the startup script executable
RUN chmod +x /app/start.sh

# Set the entrypoint to the startup script
ENTRYPOINT ["/app/start.sh"]