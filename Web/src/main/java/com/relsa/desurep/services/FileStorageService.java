package com.relsa.desurep.services;

import com.relsa.desurep.repositories.FileRepository;
import com.relsa.desurep.models.File;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import javax.sql.rowset.serial.SerialBlob;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Objects;
import java.util.UUID;

@Service
public class FileStorageService {
    @Autowired
    private FileRepository fileRepository;

    public void saveFile(MultipartFile file, String username,
                         String author, String publicationName,
                         String theme, String yearOfPublication,
                         String description, boolean isPublic) {
        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        try {
            File newFile = new File(UUID.randomUUID().toString() + file.getOriginalFilename(),
                    UUID.randomUUID().toString(), author, publicationName, theme,
                    yearOfPublication, dateFormat.format(new Date()),
                    username, file.getContentType(), description, isPublic, new SerialBlob(file.getBytes()));
            fileRepository.save(newFile);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public File getFile(String filemame, String username, int callOption) {
        File file = fileRepository.findFileByFilename(filemame);
        if ((file.isPublic() && callOption== 0) || Objects.equals(file.getUploaderName(), username))
            return file;
        return null;
    }

    public File getFileUnauth(String downloadId) {
        return fileRepository.findFileByDownloadId(downloadId);
    }

    public List<File> getFilesByUser(String username) {
        return fileRepository.findFilesByUploaderName(username);
    }

    public List<File> getPublicFiles() {
        return fileRepository.findFilesByIsPublic(true);
    }

    public void deleteFile(String filename) {
        File delFile = fileRepository.findFileByFilename(filename);
        fileRepository.delete(delFile);
    }

    public void updateFile(String filename, String author,
                           String publicationName, String theme,
                           String yearOfPublication, Boolean isPublic,
                           String description) {
        File updFile = fileRepository.findFileByFilename(filename);
        updFile.setPublicationName(publicationName);
        updFile.setPublicationDate(yearOfPublication);
        updFile.setAuthorName(author);
        updFile.setTheme(theme);
        updFile.setDescription(description);
        updFile.setPublic(isPublic);
        fileRepository.save(updFile);
    }
}
