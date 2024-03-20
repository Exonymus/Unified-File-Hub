package com.relsa.desurep.models;

import org.hibernate.annotations.Type;

import javax.persistence.*;

import java.sql.Blob;
import java.util.Objects;

@Entity
@Table(name="files")
public class File {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    private String filename;
    private String downloadId;
    private String authorName;
    private String publicationName;
    private String theme;
    private String publicationDate;
    private String description;
    private String uploadDate;
    private String uploaderName;
    private String docType;
    @Column(columnDefinition = "TINYINT")
    @Type(type = "org.hibernate.type.NumericBooleanType")
    private Boolean isPublic;
    @Lob @Basic(fetch= FetchType.LAZY)
    private Blob data;

    public File(String filename, String downloadId,
                String authorName, String publicationName,
                String theme, String publicationDate,
                String uploadDate, String uploaderName,
                String docType, String description, boolean isPublic, Blob data) {
        this.filename = filename;
        this.downloadId = downloadId;
        this.authorName = authorName;
        this.publicationName = publicationName;
        this.theme = theme;
        this.publicationDate = publicationDate;
        this.uploadDate = uploadDate;
        this.uploaderName = uploaderName;
        this.docType = docType;
        this.description = description;
        this.data = data;
        this.isPublic = isPublic;
    }

    public File(String publicationName, String authorName,
                String theme, String publicationDate,
                String uploadDate) {
        this.authorName = authorName;
        this.publicationName = publicationName;
        this.theme = theme;
        this.publicationDate = publicationDate;
        this.uploadDate = uploadDate;
    }

    public File() {}

    public String getFilename() {
        return filename;
    }

    public void setFilename(String filename) {
        this.filename = filename;
    }

    public String getDownloadId() {
        return downloadId;
    }

    public void setDownloadId(String downloadId) {
        this.downloadId = downloadId;
    }

    public String getAuthorName() {
        return authorName;
    }

    public void setAuthorName(String authorName) {
        this.authorName = authorName;
    }

    public String getPublicationName() {
        return publicationName;
    }

    public void setPublicationName(String publicationName) {
        this.publicationName = publicationName;
    }

    public String getTheme() {
        return theme;
    }

    public void setTheme(String theme) {
        this.theme = theme;
    }

    public String getPublicationDate() {
        return publicationDate;
    }

    public void setPublicationDate(String publicationDate) {
        this.publicationDate = publicationDate;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public String getUploadDate() {
        return uploadDate;
    }

    public void setUploadDate(String uploadDate) {
        this.uploadDate = uploadDate;
    }

    public String getUploaderName() {
        return uploaderName;
    }

    public void setUploaderName(String uploaderName) {
        this.uploaderName = uploaderName;
    }

    public String getDocType() {
        return docType;
    }

    public void setDocType(String docType) {
        this.docType = docType;
    }

    public boolean isPublic() { return isPublic; }

    public void setPublic(boolean aPublic) { isPublic = aPublic; }


    public Blob getData() {
        return data;
    }

    public void setData(Blob data) {
        this.data = data;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        File student = (File) o;

        if (!Objects.equals(publicationName, student.publicationName) && !Objects.equals(publicationName, "Any_value")) return false;
        if (!Objects.equals(authorName, student.authorName) && !Objects.equals(authorName, "Any_value")) return false;
        if (!Objects.equals(publicationDate, student.publicationDate) && !Objects.equals(publicationDate, "Any_value")) return false;
        if (!Objects.equals(theme, student.theme) && !Objects.equals(theme, "Any_value")) return false;
        return Objects.equals(uploadDate, student.uploadDate) || Objects.equals(uploadDate, "Any_value");
    }
}
